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

float test_p = 0;
float test_y = 0;

Planet::Planet(const World& in_world) : SceneComponent("planet"), world(in_world)
{
	root = std::make_shared<PlanetRegion>(*this, in_world, 13, 0);
	root->regenerate(15, 0.1f, 200.00);
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

void Planet::tick(double delta_time)
{
	STAT_DURATION(Planet_Tick);
	SceneComponent::tick(delta_time);
	root->tick(delta_time);
}

void Planet::render(Camera& camera)
{
	STAT_DURATION(Render_Planet);
	SceneComponent::render(camera);
	root->render(camera);
}

PlanetRegion::PlanetRegion(const Planet& in_parent, const World& in_world, uint32_t in_lod_level,
                           uint32_t in_my_level) :
	world(in_world), num_lods(in_lod_level), current_lod(in_my_level), parent(in_parent)
{
	mesh = Mesh::create("planet_lod:" + std::to_string(current_lod));

	if (current_lod + 1 < num_lods)
		child = std::make_shared<PlanetRegion>(parent, world, num_lods, in_my_level + 1);
}

static void generate_rectangle_area(std::vector<uint32_t>& indices, std::vector<Eigen::Vector3f>& positions,
                                    int32_t x_min, int32_t x_max, int32_t y_min, int32_t y_max, float scale,
                                    bool flip_direction = false)
{
	const uint32_t x_width = std::abs(x_max - x_min);
	const uint32_t y_width = std::abs(y_max - y_min);

	const float max_global = static_cast<float>(std::max(std::max(std::abs(x_min), std::abs(x_max)),
	                                                     std::max(std::abs(y_min), std::abs(y_max))));
	float min_global = static_cast<float>(std::min(std::min(std::abs(x_min), std::abs(x_max)),
	                                               std::min(std::abs(y_min), std::abs(y_max))));

	if (min_global == max_global)
		min_global = 0;

	const auto current_index_offset = static_cast<uint32_t>(positions.size());
	for (int32_t y = y_min; y <= y_max; ++y)
		for (int32_t x = x_min; x <= x_max; ++x)
		{
			const float value_local = static_cast<float>(std::max(std::abs(x), std::abs(y)));
			const float distance = (value_local - min_global) / (max_global - min_global);
			const bool orient_x = std::abs(x) > std::abs(y);
			positions.emplace_back(Eigen::Vector3f(x * scale, y * scale,
			                                       (std::abs(x) % 2 == 0 && !orient_x || std::abs(y) % 2 == 0 &&
				                                       orient_x) * distance));
		}

	if (flip_direction)
		for (uint32_t y = 0; y < y_width; ++y)
			for (uint32_t x = 0; x < x_width; ++x)
			{
				uint32_t base_index = x + y * (x_width + 1) + current_index_offset;
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + x_width + 2);
				indices.emplace_back(base_index + x_width + 1);
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 2);
			}
	else
		for (uint32_t y = 0; y < y_width; ++y)
			for (uint32_t x = 0; x < x_width; ++x)
			{
				uint32_t base_index = x + y * (x_width + 1) + current_index_offset;
				indices.emplace_back(base_index);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 1);
				indices.emplace_back(base_index + 1);
				indices.emplace_back(base_index + x_width + 2);
				indices.emplace_back(base_index + x_width + 1);
			}
}


void PlanetRegion::regenerate(int32_t in_cell_number, float in_width, double inner_radius)
{
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
		child->regenerate(cell_number, cell_size * 2, 0);
}

void PlanetRegion::tick(double delta_time)
{
	auto camera_relative_location = world.get_camera()->get_world_position();

	// @TODO Presque OK
	Eigen::Vector3d sphere_relative_location = Eigen::Vector3d(asin(camera_relative_location.x() / parent.radius),
	                                                           asin(camera_relative_location.y() / parent.radius),
	                                                           1) * parent.radius;

	// @TODO Temp
	planet_rotation = Eigen::Affine3d::Identity();
	planet_rotation.translate(-Engine::get().get_world().get_camera()->get_world_position()); // Camera is the center of the world ! yay
	planet_rotation.translate(Eigen::Vector3d(0, 0, -parent.radius));
	planet_rotation.rotate(
		Eigen::AngleAxisd(test_p, Eigen::Vector3d::UnitX()) * Eigen::AngleAxisd(test_y, Eigen::Vector3d::UnitY()));

	// @TODO Pas OK
	const auto camera_world_pos = Engine::get().get_world().get_camera()->get_world_position();
	const auto planet_center = Eigen::Vector3d(0, 0, -parent.radius);
	const auto camera_direction = (camera_world_pos - planet_center).normalized();

	Eigen::Affine3d camera_dir_matrix = Eigen::Affine3d::Identity();
	camera_dir_matrix.rotate(Eigen::Quaterniond::Identity());
	camera_dir_matrix.translate(sphere_relative_location);

	sphere_relative_location = camera_dir_matrix.translation();


	const double snapping = cell_size * 2;
	chunk_position = Eigen::Vector3d(
		std::round(sphere_relative_location.x() / snapping + 0.5) - 0.5,
		std::round(sphere_relative_location.y() / snapping + 0.5) - 0.5,
		0) * snapping;
	transform = Eigen::Affine3d::Identity();
	transform.translate(chunk_position);

	temp_location = Eigen::Affine3d::Identity();
	temp_location.translate(chunk_position);

	temp_rotation = Eigen::Affine3d::Identity();

	Eigen::AngleAxisd rotation = Eigen::AngleAxisd::Identity();

	if (current_lod != 0)
	{
		if (sphere_relative_location.x() >= chunk_position.x() && sphere_relative_location.y() < chunk_position.y())
		{
			rotation = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ());
		}
		else if (sphere_relative_location.x() < chunk_position.x() && sphere_relative_location.y() >= chunk_position.
			y())
		{
			rotation = Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitZ());
		}
		else if (sphere_relative_location.x() >= chunk_position.x() && sphere_relative_location.y() >= chunk_position.
			y())
		{
			rotation = Eigen::AngleAxisd(static_cast<float>(M_PI), Eigen::Vector3d::UnitZ());
		}
		transform.rotate(rotation);
		temp_rotation.rotate(rotation);
	}

	if (child)
		child->tick(delta_time);
}

void PlanetRegion::render(Camera& camera) const
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	Planet::get_landscape_material()->use();
	glUniform1f(glGetUniformLocation(Planet::get_landscape_material()->program_id(), "inner_width"),
	            cell_number * cell_size);
	glUniform1f(glGetUniformLocation(Planet::get_landscape_material()->program_id(), "outer_width"),
	            cell_number * cell_size * 2);
	glUniform1f(glGetUniformLocation(Planet::get_landscape_material()->program_id(), "cell_width"), cell_size);
	glUniform1i(glGetUniformLocation(Planet::get_landscape_material()->program_id(), "fragment_normals"),
	            parent.fragment_normals);
	glUniform1f(glGetUniformLocation(Planet::get_landscape_material()->program_id(), "radius"), parent.radius);
	glUniform1i(glGetUniformLocation(Planet::get_landscape_material()->program_id(), "morph_to_sphere"),
	            parent.morph_to_sphere);
	glUniformMatrix4fv(glGetUniformLocation(Planet::get_landscape_material()->program_id(), "temp_location"), 1, false,
	                   temp_location.cast<float>().matrix().data());
	glUniformMatrix4fv(glGetUniformLocation(Planet::get_landscape_material()->program_id(), "temp_rotation"), 1, false,
	                   temp_rotation.cast<float>().matrix().data());
	Planet::get_landscape_material()->set_model_transform(planet_rotation);

	const int grass_location = glGetUniformLocation(Planet::get_landscape_material()->program_id(), "grass");
	glUniform1i(grass_location, grass_location);
	grass->bind(grass_location);

	const int rock_location = glGetUniformLocation(Planet::get_landscape_material()->program_id(), "rock");
	glUniform1i(rock_location, rock_location);
	rock->bind(rock_location);

	const int sand_location = glGetUniformLocation(Planet::get_landscape_material()->program_id(), "sand");
	glUniform1i(sand_location, sand_location);
	sand->bind(sand_location);

	mesh->draw();
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glFrontFace(GL_CW);
	mesh->draw();
	glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);

	if (child)
		child->render(camera);
}

void PlanetInformations::draw()
{
	ImGui::Checkbox("Fragment Normals", &planet->fragment_normals);
	ImGui::SliderFloat("pitch : ", &test_p, -M_PI, M_PI);
	ImGui::SliderFloat("yaw : ", &test_y, -M_PI, M_PI);
	ImGui::DragFloat("radius : ", &planet->radius, 10);
	ImGui::Checkbox("is sphere : ", &planet->morph_to_sphere);
}
