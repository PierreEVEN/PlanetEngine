#include <imgui.h>

#include "camera.h"
#include "Planet.h"

#include "graphics/mesh.h"
#include "graphics/material.h"

static float width = 5;
static std::shared_ptr<Material> planet_material = nullptr;

Planet::Planet(const World& in_world) : world(in_world)
{
	root = std::make_shared<PlanetRegion>(in_world, 0);
	root->regenerate(3, width, 200.00, Eigen::Vector3d(0, 0, 0));
}

std::shared_ptr<Material> Planet::get_landscape_material()
{
	if (planet_material)
		return planet_material;
	planet_material = Material::create("standard_material");
	planet_material->load_from_source("resources/shaders/standard_material.vs",
	                                  "resources/shaders/standard_material.fs");
	return planet_material;
}

void Planet::tick(double delta_time)
{
	SceneComponent::tick(delta_time);

	Eigen::Vector3d world_origin = world.get_camera()->get_world_position();
	world_origin.z() = 0;

	root->chunk_position = Eigen::Vector3d(
		std::round(world_origin.x() / width),
		std::round(world_origin.y() / width),
		std::round(world_origin.z() / width)) * width;
	root->parent_position = world_origin;

	root->tick(delta_time);
}

void Planet::render()
{
	SceneComponent::render();

	root->render();
}

PlanetRegion::PlanetRegion(const World& in_world, uint32_t in_lod_level) :
	world(in_world), lod_level(in_lod_level), mesh(Mesh::create())
{
	if (lod_level != 0)
		child = std::make_shared<PlanetRegion>(world, lod_level - 1);
}

static void generate_rectangle_area(std::vector<uint32_t>& indices, std::vector<Eigen::Vector3f>& positions,
                                    int32_t x_min, int32_t x_max, int32_t y_min, int32_t y_max, float scale,
                                    bool flip_direction = false)
{
	const uint32_t x_width = std::abs(x_max - x_min);
	const uint32_t y_width = std::abs(y_max - y_min);

	// RIGHT side (larger)
	const auto current_index_offset = static_cast<uint32_t>(positions.size());
	for (int32_t y = y_min; y <= y_max; ++y)
		for (int32_t x = x_min; x <= x_max; ++x)
			positions.emplace_back(Eigen::Vector3f(x * scale, y * scale, 0));

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


void PlanetRegion::regenerate(int32_t cell_number, float in_width, double inner_radius,
                              const Eigen::Vector3d& in_position)
{
	cell_size = in_width;
	chunk_position = in_position;

	std::vector<Eigen::Vector3f> positions(11);
	std::vector<Eigen::Vector3f> normals;
	std::vector<Eigen::Vector2f> texture_coordinates;
	std::vector<Eigen::Vector3f> colors;

	positions[0] = Eigen::Vector3d(-inner_radius - cell_size, inner_radius + cell_size, 0).cast<float>(); // A
	positions[1] = Eigen::Vector3d(inner_radius + cell_size, inner_radius + cell_size, 0).cast<float>(); // B 
	positions[2] = Eigen::Vector3d(inner_radius + cell_size, -inner_radius - cell_size, 0).cast<float>(); // C
	positions[3] = Eigen::Vector3d(-inner_radius - cell_size, -inner_radius - cell_size, 0).cast<float>(); // D

	positions[4] = Eigen::Vector3d(-inner_radius, inner_radius, 0).cast<float>(); // E
	positions[5] = Eigen::Vector3d(inner_radius, inner_radius, 0).cast<float>(); // F
	positions[6] = Eigen::Vector3d(inner_radius, -inner_radius, 0).cast<float>(); // G
	positions[7] = Eigen::Vector3d(-inner_radius, -inner_radius, 0).cast<float>(); // H

	positions[8] = Eigen::Vector3d(-inner_radius, inner_radius - cell_size / 2, 0).cast<float>(); // I
	positions[9] = Eigen::Vector3d(inner_radius - cell_size / 2, inner_radius - cell_size / 2, 0).cast<float>(); // J
	positions[10] = Eigen::Vector3d(inner_radius - cell_size / 2, -inner_radius, 0).cast<float>(); // K

	std::vector<uint32_t> indices = {
		0, 5, 1, 0, 4, 5,
		1, 6, 2, 1, 5, 6,
		2, 7, 3, 2, 6, 7,
		3, 4, 0, 3, 7, 4,
		4, 9, 5, 4, 8, 9,
		5, 10, 6, 5, 9, 10
	};

	positions.clear();
	indices.clear();

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
	                        cell_number + 1,
	                        cell_number,
	                        cell_number * 2 + 1,
	                        cell_size, true);

	// BOTTOM side (larger)
	generate_rectangle_area(indices, positions,
	                        -cell_number * 2 - 1,
	                        -cell_number - 1,
	                        -cell_number * 2 - 1,
	                        cell_number + 1,
	                        cell_size);

	// LEFT side (larger)
	generate_rectangle_area(indices, positions,
	                        -cell_number - 1,
	                        cell_number * 2 + 1,
	                        -cell_number * 2 - 1,
	                        -cell_number - 1,
	                        cell_size, true);

	for (int i = 0; i < positions.size(); ++i)
	{
		normals.emplace_back(Eigen::Vector3f(
			static_cast<float>(fmod(std::abs(sin((lod_level + 1) * 3678.45678)), 1.0)),
			static_cast<float>(fmod(std::abs(sin((lod_level + 1) * 98.0987654)), 1.0)),
			static_cast<float>(fmod(std::abs(sin((lod_level + 1) * 567.845496)), 1.0))));
		texture_coordinates.emplace_back(Eigen::Vector2f(0, 0));
	}

	mesh->set_positions(positions, 0, true);
	mesh->set_normals(normals, 1, true);
	mesh->set_texture_coordinates(texture_coordinates, 2, true);
	mesh->set_indices(indices);

	if (child)
		child->regenerate(cell_number, cell_size * 2, 0,
		                  chunk_position - Eigen::Vector3d(cell_size / 4, cell_size / 4, 0));
}

void PlanetRegion::tick(double delta_time)
{
	transform = Eigen::Affine3d::Identity();
	transform.translate(chunk_position); // +Eigen::Vector3d(cell_size, -cell_size, cell_size));

	Eigen::AngleAxisd rotation = Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ());
	if (parent_position.x() >= chunk_position.x() && parent_position.y() < chunk_position.y())
	{
		rotation = Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ());
	}
	else if (parent_position.x() < chunk_position.x() && parent_position.y() >= chunk_position.y())
	{
		rotation = Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ());
	}
	else if (parent_position.x() && parent_position.y() < chunk_position.y())
	{
		rotation = Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitZ());
	}
	else if (parent_position.x() >= chunk_position.x() && parent_position.y() >= chunk_position.y())
	{
		rotation = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ());
	}
	// std::cout << "cell : " << cell_size << " / pos : " << chunk_position.x() + 20 << " -> cam : " << parent_position.x() << std::endl;


	transform.rotate(rotation);

	if (child)
	{
		Eigen::Affine3d child_transform = Eigen::Affine3d::Identity();
		child_transform.rotate(rotation);
		child_transform.translate(Eigen::Vector3d(cell_size * 2, -cell_size * 2, 0));
		child->parent_position = chunk_position;
		child->chunk_position = chunk_position + child_transform.translation();
		child->tick(delta_time);
	}
}

void PlanetRegion::render()
{
	if (child)
		child->render();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	Planet::get_landscape_material()->use();
	Planet::get_landscape_material()->set_model_transform(transform);
	mesh->draw();
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glFrontFace(GL_CW);
	mesh->draw();
	glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
}
