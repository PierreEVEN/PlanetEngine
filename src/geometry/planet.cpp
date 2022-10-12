#include <GL/gl3w.h>

#include "camera.h"
#include "planet.h"

#include <imgui.h>
#include <iostream>

#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/compute_shader.h"
#include "graphics/mesh.h"
#include "graphics/material.h"
#include "graphics/texture_image.h"
#include "utils/profiler.h"

static std::shared_ptr<Material> planet_material = nullptr;
static std::shared_ptr<TextureImage> grass = nullptr;
static std::shared_ptr<TextureImage> rock = nullptr;
static std::shared_ptr<TextureImage> sand = nullptr;
static std::shared_ptr<ComputeShader> compute_positions = nullptr;
static std::shared_ptr<ComputeShader> compute_normals = nullptr;


static double snap(double value, double delta) { return round(value / delta) * delta; }

Planet::Planet(const World& in_world) : SceneComponent("planet"), world(in_world)
{
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


	compute_positions = ComputeShader::create("Planet compute position");
	compute_positions->load_from_source("resources/shaders/compute/planet_compute_position.cs");

	compute_normals = ComputeShader::create("Planet compute normals");
	compute_normals->load_from_source("resources/shaders/compute/planet_compute_normals.cs");

	return planet_material;
}

static void generate_rectangle_area(std::vector<uint32_t>& indices, std::vector<Eigen::Vector3f>& positions,
                                    int32_t x_min, int32_t x_max, int32_t z_min, int32_t z_max,
                                    int32_t cell_min, int32_t cell_max)
{
	// Requirement for Y val
	const float distance_max = static_cast<float>(cell_max);
	float min_global = static_cast<float>(cell_min);

	// Handle LOD 0 case
	if (min_global == distance_max)
		min_global = 0;

	const auto current_index_offset = static_cast<uint32_t>(positions.size());
	for (int32_t z = z_min; z <= z_max; ++z)
		for (int32_t x = x_min; x <= x_max; ++x)
		{
			// The Y val is used to store the progression from previous LOD to the next one
			const float Linf_distance = static_cast<float>(std::max(std::abs(x), std::abs(z))); // Tchebychev distance
			const float y_val_weight = (Linf_distance - min_global) / (distance_max - min_global);
			const bool x_aligned = std::abs(x) > std::abs(z);
			const int mask = std::abs(x) % 2 == 0 && !x_aligned || std::abs(z) % 2 == 0 && x_aligned;
			positions.emplace_back(Eigen::Vector3f(static_cast<float>(x), mask * y_val_weight, static_cast<float>(z)));
		}

	const uint32_t x_width = std::abs(x_max - x_min);
	const uint32_t z_width = std::abs(z_max - z_min);
	for (uint32_t z = 0; z < z_width; ++z)
		for (uint32_t x = 0; x < x_width; ++x)
		{
			uint32_t base_index = x + z * (x_width + 1) + current_index_offset;
			if (positions[base_index].x() * positions[base_index].z() > 0)
			{
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + x_width + 2);
				indices.emplace_back(base_index + x_width + 1);
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 2);
			}
			else
			{
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 1);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 2);
				indices.emplace_back(base_index + x_width + 1);
			}
		}
}

void Planet::regenerate()
{
	std::vector<Eigen::Vector3f> positions_root;
	std::vector<uint32_t> indices_root;

	generate_rectangle_area(indices_root, positions_root,
	                        -cell_count * 2 - 1,
	                        cell_count * 2 + 1,
	                        -cell_count * 2 - 1,
	                        cell_count * 2 + 1,
	                        0, cell_count * 2);

	root_mesh = Mesh::create("planet root mesh");
	root_mesh->set_positions(positions_root, 0, true);
	root_mesh->set_indices(indices_root);

	std::vector<Eigen::Vector3f> positions_child;
	std::vector<uint32_t> indices_child;
	// TOP side (larger)
	generate_rectangle_area(indices_child, positions_child,
	                        cell_count,
	                        cell_count * 2 + 1,
	                        -cell_count - 1,
	                        cell_count * 2 + 1,
	                        cell_count, cell_count * 2 + 1);

	// RIGHT side (larger)
	generate_rectangle_area(indices_child, positions_child,
	                        -cell_count * 2 - 1,
	                        cell_count,
	                        cell_count,
	                        cell_count * 2 + 1,
	                        cell_count, cell_count * 2 + 1);

	// BOTTOM side
	generate_rectangle_area(indices_child, positions_child,
	                        -cell_count * 2 - 1,
	                        -cell_count - 1,
	                        -cell_count * 2 - 1,
	                        cell_count,
	                        cell_count + 1, cell_count * 2 + 1);

	// LEFT side
	generate_rectangle_area(indices_child, positions_child,
	                        -cell_count - 1,
	                        cell_count * 2 + 1,
	                        -cell_count * 2 - 1,
	                        -cell_count - 1,
	                        cell_count + 1, cell_count * 2 + 1);

	child_mesh = Mesh::create("planet child mesh");
	child_mesh->set_positions(positions_child, 0, true);
	child_mesh->set_indices(indices_child);

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
		planet_inverse_rotation = planet_orientation.inverse();

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
	world(in_world), num_lods(in_lod_level), current_lod(in_my_level), planet(in_parent)
{
}


void PlanetRegion::regenerate(int32_t in_cell_number, double in_width)
{
	STAT_DURATION("regenerate planet LOD" + std::to_string(current_lod));
	cell_number = in_cell_number;
	cell_size = in_width; // in_width;

	if (child)
		child->regenerate(cell_number, cell_size * 2);
}


void PlanetRegion::tick(double delta_time, int in_num_lods)
{
	// Create or destroy children
	num_lods = in_num_lods;
	if (!child && current_lod + 1 < num_lods)
	{
		child = std::make_shared<PlanetRegion>(planet, world, num_lods, current_lod + 1);
		child->regenerate(cell_number, cell_size * 2);
	}
	if (child && current_lod >= num_lods - 1)
		child = nullptr;

	if (child)
		child->tick(delta_time, num_lods);

	STAT_DURATION("Planet Tick LOD :" + std::to_string(current_lod));

	// Compute camera position in local space
	const Eigen::Vector3d camera_local_position = planet.planet_inverse_rotation * (world.get_camera()->
		get_world_position() - planet.get_world_position());


	const Eigen::Vector3d temp = Eigen::Vector3d(
		0,
		Eigen::Vector3d(camera_local_position.x(), camera_local_position.y(), 0).normalized().y(),
		Eigen::Vector3d(camera_local_position.x(), camera_local_position.y(), camera_local_position.z()).normalized().
		z()
	);


	// Convert linear position to position on sphere // @TODO Minor fix required here
	Eigen::Vector3d local_location = Eigen::Vector3d(
		0,
		asin(std::clamp(temp.y(), -1.0, 1.0)),
		asin(std::clamp(temp.z(), -1.0, 1.0))) * planet.radius;

	const double snapping = cell_size * 2;
	chunk_position = Eigen::Vector3d(
		std::round(local_location.y() / snapping + 0.5) - 0.5,
		0,
		std::round(local_location.z() / snapping + 0.5) - 0.5) * snapping;


	lod_local_transform = Eigen::Affine3d::Identity();
	lod_local_transform.translate(chunk_position);
	lod_local_transform.scale(cell_size);

	Eigen::AngleAxisd rotation = Eigen::AngleAxisd::Identity();

	if (current_lod != 0)
	{
		if (local_location.y() >= chunk_position.x() && local_location.z() < chunk_position.z())
			rotation = Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitY());
		else if (local_location.y() < chunk_position.x() && local_location.z() >= chunk_position.z())
			rotation = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitY());
		else if (local_location.y() >= chunk_position.x() && local_location.z() >= chunk_position.z())
			rotation = Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitY());
		lod_local_transform.rotate(rotation);
	}
}

void PlanetRegion::render(Camera& camera) const
{
	if (child)
		child->render(camera);
	STAT_DURATION("Render planet lod " + std::to_string(current_lod));
	// Set uniforms
	Planet::get_landscape_material()->bind();
	glUniform1f(3, planet.radius);

	glUniform1f(Planet::get_landscape_material()->binding("cell_width"), planet.cell_width);

	glUniform3fv(Planet::get_landscape_material()->binding("ground_color"), 1, planet.planet_color.data());

	glUniformMatrix4fv(Planet::get_landscape_material()->binding("lod_local_transform"), 1, false,
	                   lod_local_transform.cast<float>().matrix().data());

	Planet::get_landscape_material()->set_model_transform(planet.planet_global_transform);

	// Bind textures
	const int grass_location = Planet::get_landscape_material()->binding("grass");
	glUniform1i(grass_location, grass_location);
	grass->bind(grass_location);

	const int rock_location = Planet::get_landscape_material()->binding("rock");
	glUniform1i(rock_location, rock_location);
	rock->bind(rock_location);

	const int sand_location = Planet::get_landscape_material()->binding("sand");
	glUniform1i(sand_location, sand_location);
	sand->bind(sand_location);

	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, Engine::get().get_renderer().wireframe ? GL_LINE : GL_FILL);
	if (current_lod == 0)
		planet.root_mesh->draw();
	else
		planet.child_mesh->draw();
	if (planet.double_sided)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glFrontFace(GL_CW);
		if (current_lod == 0)
			planet.root_mesh->draw();
		else
			planet.child_mesh->draw();
		glFrontFace(GL_CCW);
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void PlanetInformations::draw()
{
	ImGui::Checkbox("Double sided", &planet->double_sided);
	ImGui::Checkbox("Fragment Normals", &planet->fragment_normals);
	ImGui::SliderInt("num LODs : ", &planet->num_lods, 1, 40);
	ImGui::DragFloat("radius : ", &planet->radius, 10);
	if (ImGui::SliderInt("cell number", &planet->cell_count, 1, 40) ||
		ImGui::SliderFloat("cell_width : ", &planet->cell_width, 0.05f, 10))
		planet->regenerate();
	ImGui::Separator();
	ImGui::ColorPicker3("color", planet->planet_color.data());
}
