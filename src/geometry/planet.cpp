#include <GL/gl3w.h>

#include "camera.h"
#include "planet.h"

#include <imgui.h>

#include "engine/engine.h"
#include "graphics/mesh.h"
#include "graphics/material.h"
#include "graphics/texture_image.h"
#include "utils/profiler.h"

static std::shared_ptr<Material> planet_material = nullptr;
static std::shared_ptr<TextureImage> grass = nullptr;
static std::shared_ptr<TextureImage> rock = nullptr;
static std::shared_ptr<TextureImage> sand = nullptr;

static double snap(double value, double delta) { return round(value / delta) * delta; }

Planet::Planet(const World& in_world) : SceneComponent("planet"), world(in_world)
{
	set_local_position({-radius, 0, 0});
	root = std::make_shared<PlanetRegion>(*this, in_world, 16, 0);
	regenerate();
	ImGuiWindow::create_window<PlanetInformations>(this);
}

std::shared_ptr<Material> Planet::get_landscape_material()
{
	if (planet_material)
		return planet_material;
	planet_material = Material::create("planet material");
	planet_material->load_from_source("resources/shaders/planet_material_v2.vs",
	                                  "resources/shaders/planet_material_v2.fs");

	grass = TextureImage::create("terrain grass", {GL_REPEAT});
	grass->load("resources/textures/terrain/grass.jpg");
	rock = TextureImage::create("terrain rock", {GL_REPEAT});
	rock->load("resources/textures/terrain/rock_diffuse.jpg");
	sand = TextureImage::create("terrain sand", {GL_REPEAT});
	sand->load("resources/textures/terrain/sand_diffuse.jpg");

	return planet_material;
}

void Planet::regenerate()
{
	root->regenerate(cell_count, cell_width);
}

void Planet::tick(double delta_time)
{
	STAT_DURATION("Planet_Tick");
	SceneComponent::tick(delta_time);


	{
		STAT_DURATION("compute planet global transform");
		// Get camera direction from planet center
		const auto camera_direction = (Engine::get().get_world().get_camera()->get_world_position() -
			get_world_position()).normalized();

		// Compute global rotation snapping step
		const double max_cell_radian_step = cell_width * std::pow(2, num_lods) / (radius * 2);

		// Compute global planet rotation (orient planet mesh toward camera)
		const auto pitch = asin(camera_direction.z());
		const auto yaw = atan2(camera_direction.y(), camera_direction.x());
		const auto planet_orientation =
			Eigen::Affine3d(
				Eigen::AngleAxisd(snap(yaw, max_cell_radian_step), Eigen::Vector3d::UnitZ()) *
				Eigen::AngleAxisd(snap(-pitch, max_cell_radian_step), Eigen::Vector3d::UnitY())
			);

		// Compute global planet transformation (ensure ground is always close to origin)
		planet_global_transform = Eigen::Affine3d::Identity();
		planet_global_transform.translate(
			get_world_position() -
			Engine::get().get_world().get_camera()->get_world_position()
		);
		planet_global_transform = planet_global_transform * planet_orientation;
		planet_global_transform.translate(Eigen::Vector3d(radius, 0, 0));
	}


	root->tick(delta_time, num_lods);
}

void Planet::render(Camera& camera)
{
	STAT_DURATION("Render Planet");
	SceneComponent::render(camera);
	root->render(camera);
}

PlanetRegion::PlanetRegion(const Planet& in_parent, const World& in_world, uint32_t in_lod_level,
                           uint32_t in_my_level) :
	world(in_world), num_lods(in_lod_level), current_lod(in_my_level), parent(in_parent)
{
	mesh = Mesh::create("planet_lod:" + std::to_string(current_lod));
}

static void generate_rectangle_area(std::vector<uint32_t>& indices, std::vector<Eigen::Vector3f>& positions,
                                    int32_t x_min, int32_t x_max, int32_t z_min, int32_t z_max, double scale,
                                    bool flip_direction = false)
{
	const uint32_t x_width = std::abs(x_max - x_min);
	const uint32_t z_width = std::abs(z_max - z_min);

	const float max_global = static_cast<float>(std::max(std::max(std::abs(x_min), std::abs(x_max)),
	                                                     std::max(std::abs(z_min), std::abs(z_max))));
	float min_global = static_cast<float>(std::min(std::min(std::abs(x_min), std::abs(x_max)),
	                                               std::min(std::abs(z_min), std::abs(z_max))));

	if (min_global == max_global)
		min_global = 0;

	const auto current_index_offset = static_cast<uint32_t>(positions.size());
	for (int32_t z = z_min; z <= z_max; ++z)
		for (int32_t x = x_min; x <= x_max; ++x)
		{
			const float value_local = static_cast<float>(std::max(std::abs(x), std::abs(z)));
			const float distance = (value_local - min_global) / (max_global - min_global);
			const bool orient_x = std::abs(x) > std::abs(z);
			positions.emplace_back(Eigen::Vector3f(x * static_cast<float>(scale),
			                                       (std::abs(x) % 2 == 0 && !orient_x || std::abs(z) % 2 == 0 &&
				                                       orient_x) * distance,
			                                       z * static_cast<float>(scale)));
		}

	if (flip_direction)
		for (uint32_t z = 0; z < z_width; ++z)
			for (uint32_t x = 0; x < x_width; ++x)
			{
				uint32_t base_index = x + z * (x_width + 1) + current_index_offset;
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + x_width + 2);
				indices.emplace_back(base_index + x_width + 1);
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 2);
			}
	else
		for (uint32_t z = 0; z < z_width; ++z)
			for (uint32_t x = 0; x < x_width; ++x)
			{
				uint32_t base_index = x + z * (x_width + 1) + current_index_offset;
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 1);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 2);
				indices.emplace_back(base_index + x_width + 1);
			}
}


void PlanetRegion::regenerate(int32_t in_cell_number, double in_width)
{
	STAT_DURATION("regenerate planet LOD" + std::to_string(current_lod));
	cell_number = in_cell_number;
	cell_size = in_width;

	std::vector<Eigen::Vector3f> positions;
	std::vector<uint32_t> indices;

	if (current_lod == 0)
	{
		generate_rectangle_area(indices, positions,
		                        -cell_number * 2 - 1,
		                        cell_number * 2 + 1,
		                        -cell_number * 2 - 1,
		                        cell_number * 2 + 1,
		                        cell_size);
	}
	else
	{
		// TOP side (larger)
		generate_rectangle_area(indices, positions,
		                        cell_number,
		                        cell_number * 2 + 1,
		                        -cell_number - 1,
		                        cell_number * 2 + 1,
		                        cell_size);

		// RIGHT side (larger)
		generate_rectangle_area(indices, positions,
		                        -cell_number * 2 - 1,
		                        cell_number,
		                        cell_number,
		                        cell_number * 2 + 1,
		                        cell_size, true);

		// BOTTOM side
		generate_rectangle_area(indices, positions,
		                        -cell_number * 2 - 1,
		                        -cell_number - 1,
		                        -cell_number * 2 - 1,
		                        cell_number,
		                        cell_size);

		// LEFT side
		generate_rectangle_area(indices, positions,
		                        -cell_number - 1,
		                        cell_number * 2 + 1,
		                        -cell_number * 2 - 1,
		                        -cell_number - 1,
		                        cell_size, true);
	}

	mesh->set_positions(positions, 0, true);
	mesh->set_indices(indices);

	if (child)
		child->regenerate(cell_number, cell_size * 2);
}


void PlanetRegion::tick(double delta_time, int in_num_lods)
{
	// Create or destroy children
	num_lods = in_num_lods;
	if (!child && current_lod + 1 < num_lods)
	{
		child = std::make_shared<PlanetRegion>(parent, world, num_lods, current_lod + 1);
		child->regenerate(cell_number, cell_size * 2);
	}
	if (child && current_lod >= num_lods - 1)
		child = nullptr;

	if (child)
		child->tick(delta_time, num_lods);

	STAT_DURATION("Planet Tick_LOD :" + std::to_string(current_lod));
	const Eigen::Vector3d camera_relative_location = parent.planet_global_transform.inverse() * -world.get_camera()->
		get_world_position();
	
	// @TODO Presque OK
	Eigen::Vector3d sphere_relative_location = Eigen::Vector3d(
		0,
		asin(std::clamp(camera_relative_location.y() / parent.radius, -1.0, 1.0)),
		asin(std::clamp(camera_relative_location.z() / parent.radius, -1.0, 1.0))) * parent.radius;

	sphere_relative_location = Eigen::Vector3d::Identity();

	const double snapping = cell_size * 2;
	chunk_position = Eigen::Vector3d(
		std::round(sphere_relative_location.x() / snapping + 0.5) - 0.5,
		0,
		std::round(sphere_relative_location.z() / snapping + 0.5) - 0.5) * snapping;

	lod_local_transform = Eigen::Affine3d::Identity();
	lod_local_transform.translate(chunk_position);

	Eigen::AngleAxisd rotation = Eigen::AngleAxisd::Identity();

	if (current_lod != 0)
	{
		if (sphere_relative_location.x() >= chunk_position.x() && sphere_relative_location.z() < chunk_position.z())
		{
			rotation = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ());
		}
		else if (sphere_relative_location.x() < chunk_position.x() && sphere_relative_location.z() >= chunk_position.
			z())
		{
			rotation = Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitZ());
		}
		else if (sphere_relative_location.x() >= chunk_position.x() && sphere_relative_location.z() >= chunk_position.
			z())
		{
			rotation = Eigen::AngleAxisd(static_cast<float>(M_PI), Eigen::Vector3d::UnitZ());
		}
		lod_local_transform.rotate(rotation);
	}

}

void PlanetRegion::render(Camera& camera) const
{
	if (child)
		child->render(camera);
	STAT_DURATION("Render planet lod " + std::to_string(current_lod));
	// Set uniforms
	Planet::get_landscape_material()->use();
	glUniform1f(
		glGetUniformLocation(Planet::get_landscape_material()->program_id(), "radius"),
		parent.radius);

	glUniform3fv(
		glGetUniformLocation(Planet::get_landscape_material()->program_id(), "ground_color"), 1,
		parent.planet_color.data());

	glUniformMatrix4fv(
		glGetUniformLocation(Planet::get_landscape_material()->program_id(), "lod_local_transform"),
		1, false, lod_local_transform.cast<float>().matrix().data());

	Planet::get_landscape_material()->set_model_transform(parent.planet_global_transform);

	// Bind textures
	const int grass_location = glGetUniformLocation(Planet::get_landscape_material()->program_id(), "grass");
	glUniform1i(grass_location, grass_location);
	grass->bind(grass_location);

	const int rock_location = glGetUniformLocation(Planet::get_landscape_material()->program_id(), "rock");
	glUniform1i(rock_location, rock_location);
	rock->bind(rock_location);

	const int sand_location = glGetUniformLocation(Planet::get_landscape_material()->program_id(), "sand");
	glUniform1i(sand_location, sand_location);
	sand->bind(sand_location);

	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, parent.wire_frame ? GL_LINE : GL_FILL);
	mesh->draw();
	if (parent.double_sided) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glFrontFace(GL_CW);
		mesh->draw();
		glFrontFace(GL_CCW);
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

}

void PlanetInformations::draw()
{
	ImGui::Checkbox("Wire frame", &planet->wire_frame);
	ImGui::Checkbox("Double sided", &planet->double_sided);
	ImGui::Checkbox("Fragment Normals", &planet->fragment_normals);
	ImGui::Checkbox("Fragment Normals", &planet->fragment_normals);
	ImGui::SliderInt("num LODs : ", &planet->num_lods, 1, 20);
	ImGui::DragFloat("radius : ", &planet->radius, 10);
	if (ImGui::SliderInt("cell number", &planet->cell_count, 2, 40) ||
		ImGui::SliderFloat("cell_width : ", &planet->cell_width, 0.05f, 10))
		planet->regenerate();
	ImGui::Separator();
	ImGui::ColorPicker3("color", planet->planet_color.data());
}
