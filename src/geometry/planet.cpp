#include "Planet.h"

#include "graphics/mesh.h"
#include "graphics/material.h"

Planet::Planet()
{
	root = std::make_shared<PlanetRegion>(5, 130, 200, Eigen::Vector3d(0, 0, 0), RegionOrientation::NE);
}

Planet::~Planet()
{
}

void Planet::tick(double delta_time)
{
	SceneComponent::tick(delta_time);
}

void Planet::render()
{
	SceneComponent::render();

	root->render();
}

PlanetRegion::PlanetRegion(int subdivision_level, double width, double inner_radius, const Eigen::Vector3d& position,
                           RegionOrientation orientation)
{
	if (subdivision_level > 0)
		child = std::make_shared<PlanetRegion>(subdivision_level - 1, width / 2, inner_radius - width * 0.75,
		                                       position - Eigen::Vector3d(width / 4, width / 4, 0), orientation);

	std::vector<Eigen::Vector3f> positions(11);
	std::vector<Eigen::Vector3f> normals(11);
	std::vector<Eigen::Vector2f> tcs(11);


	positions[0] = (position + Eigen::Vector3d(-inner_radius - width, inner_radius + width, 0)).cast<float>(); // A
	positions[1] = (position + Eigen::Vector3d(inner_radius + width, inner_radius + width, 0)).cast<float>(); // B 
	positions[2] = (position + Eigen::Vector3d(inner_radius + width, -inner_radius - width, 0)).cast<float>(); // C
	positions[3] = (position + Eigen::Vector3d(-inner_radius - width, -inner_radius - width, 0)).cast<float>(); // D

	positions[4] = (position + Eigen::Vector3d(-inner_radius, inner_radius, 0)).cast<float>(); // E
	positions[5] = (position + Eigen::Vector3d(inner_radius, inner_radius, 0)).cast<float>(); // F
	positions[6] = (position + Eigen::Vector3d(inner_radius, -inner_radius, 0)).cast<float>(); // G
	positions[7] = (position + Eigen::Vector3d(-inner_radius, -inner_radius, 0)).cast<float>(); // H

	positions[8] = (position + Eigen::Vector3d(-inner_radius, inner_radius - width / 2, 0)).cast<float>(); // I
	positions[9] = (position + Eigen::Vector3d(inner_radius - width / 2, inner_radius - width / 2, 0)).cast<float>();
	// J
	positions[10] = (position + Eigen::Vector3d(inner_radius - width / 2, -inner_radius, 0)).cast<float>(); // K

	for (int i = 0; i < 11; ++i) {
		normals[i] = Eigen::Vector3f(fmod(std::abs(sin(subdivision_level * 3678.45678)), 1.0),
			fmod(std::abs(sin(subdivision_level * 98.0987654)), 1.0),
			fmod(std::abs(sin(subdivision_level * 567.845496)), 1.0));
		tcs[i] = Eigen::Vector2f(0, 0);
	}

	const std::vector<uint32_t> indices = {
		0, 5, 1, 0, 4, 5,
		1, 6, 2, 1, 5, 6,
		2, 7, 3, 2, 6, 7,
		3, 4, 0, 3, 7, 4,
		4, 9, 5, 4, 8, 9,
		5, 10, 6, 5, 9, 10
	};

	mesh = Mesh::create();
	mesh->set_positions(positions, 0, true);
	mesh->set_normals(normals, 1, true);
	mesh->set_texture_coordinates(tcs, 2, true);
	mesh->set_indices(indices);

	material = Material::create("standard_material");
	material->load_from_source("resources/shaders/standard_material.vs",
	                           "resources/shaders/standard_material.fs");
}

void PlanetRegion::render() const
{
	if (child)
		child->render();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	material->use();
	material->set_model_transform(Eigen::Affine3d::Identity());
	mesh->draw();
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glFrontFace(GL_CW);
	mesh->draw();
	glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
}
