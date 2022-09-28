#include <imgui.h>

#include "camera.h"
#include "Planet.h"

#include "graphics/mesh.h"
#include "graphics/material.h"

double width = 13.33;
static std::shared_ptr<Material> planet_material = nullptr;

Planet::Planet(const World& in_world) : world(in_world)
{
	root = std::make_shared<PlanetRegion>(in_world, 0);
	root->regenerate(10, width, 20.00, Eigen::Vector3d(0, 0, 0));
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

Eigen::Vector3d world_origin;

void Planet::tick(double delta_time)
{
	SceneComponent::tick(delta_time);

	world_origin = world.get_camera()->get_world_position();
	world_origin.z() = 0;

	root->position = Eigen::Vector3d(
		std::round(world_origin.x() / width),
		std::round(world_origin.y() / width),
		std::round(world_origin.z() / width)) * width;

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

void PlanetRegion::regenerate(uint32_t subdivision, double in_width, double inner_radius,
                              const Eigen::Vector3d& in_position)
{
	width = in_width;
	position = in_position;

	std::vector<Eigen::Vector3f> positions(11);
	std::vector<Eigen::Vector3f> normals(11);
	std::vector<Eigen::Vector2f> texture_coordinates(11);
	std::vector<Eigen::Vector3f> colors(11);


	positions[0] = Eigen::Vector3d(-inner_radius - width, inner_radius + width, 0).cast<float>(); // A
	positions[1] = Eigen::Vector3d(inner_radius + width, inner_radius + width, 0).cast<float>(); // B 
	positions[2] = Eigen::Vector3d(inner_radius + width, -inner_radius - width, 0).cast<float>(); // C
	positions[3] = Eigen::Vector3d(-inner_radius - width, -inner_radius - width, 0).cast<float>(); // D

	positions[4] = Eigen::Vector3d(-inner_radius, inner_radius, 0).cast<float>(); // E
	positions[5] = Eigen::Vector3d(inner_radius, inner_radius, 0).cast<float>(); // F
	positions[6] = Eigen::Vector3d(inner_radius, -inner_radius, 0).cast<float>(); // G
	positions[7] = Eigen::Vector3d(-inner_radius, -inner_radius, 0).cast<float>(); // H

	positions[8] = Eigen::Vector3d(-inner_radius, inner_radius - width / 2, 0).cast<float>(); // I
	positions[9] = Eigen::Vector3d(inner_radius - width / 2, inner_radius - width / 2, 0).cast<float>(); // J
	positions[10] = Eigen::Vector3d(inner_radius - width / 2, -inner_radius, 0).cast<float>(); // K

	for (int i = 0; i < 11; ++i)
	{
		normals[i] = Eigen::Vector3f(
			static_cast<float>(fmod(std::abs(sin((lod_level + 1) * 3678.45678)), 1.0)),
			static_cast<float>(fmod(std::abs(sin((lod_level + 1) * 98.0987654)), 1.0)),
			static_cast<float>(fmod(std::abs(sin((lod_level + 1) * 567.845496)), 1.0)));
		texture_coordinates[i] = Eigen::Vector2f(0, 0);
	}

	const std::vector<uint32_t> indices = {
		0, 5, 1, 0, 4, 5,
		1, 6, 2, 1, 5, 6,
		2, 7, 3, 2, 6, 7,
		3, 4, 0, 3, 7, 4,
		4, 9, 5, 4, 8, 9,
		5, 10, 6, 5, 9, 10
	};

	mesh->set_positions(positions, 0, true);
	mesh->set_normals(normals, 1, true);
	mesh->set_texture_coordinates(texture_coordinates, 2, true);
	mesh->set_indices(indices);

	if (child)
		child->regenerate(subdivision, width / 2, inner_radius - width * 0.75,
		                  position - Eigen::Vector3d(width / 4, width / 4, 0));
}

void PlanetRegion::tick(double delta_time)
{
	transform = Eigen::Affine3d::Identity();
	transform.translate(position);

	Eigen::AngleAxisd rotation = Eigen::AngleAxisd::Identity();
	if (world_origin.x() >= position.x() && world_origin.y() < position.y())
		rotation = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ());
	else if (world_origin.x() < position.x() && world_origin.y() >= position.y())
		rotation = Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitZ());
	else if (world_origin.x() < position.x() && world_origin.y() < position.y())
		rotation = Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ());
	else if (world_origin.x() >= position.x() && world_origin.y() >= position.y())
		rotation = Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ());
	transform.rotate(rotation);

	if (child)
	{
		Eigen::Affine3d child_transform = Eigen::Affine3d::Identity();
		child_transform.rotate(rotation);
		child_transform.translate(Eigen::Vector3d(width / 4, width / 4, 0));

		child->position = position - child_transform.translation();
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
