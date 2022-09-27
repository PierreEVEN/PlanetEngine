#include "planet.h"

#include <imgui.h>

#include "graphics/mesh.h"
#include "graphics/material.h"

Planet::Planet()
{
}

Planet::~Planet()
{
}

std::atomic_uint32_t subdivision_progress;

void Planet::tick(double delta_time) {
	SceneComponent::tick(delta_time);
}

void Planet::render()
{
	SceneComponent::render();
}

PlanetRegion::PlanetRegion(int subdivision_level, double width, double inner_radius)
{
	if (subdivision_level > 0) {
		child = std::make_shared<PlanetRegion>(subdivision_level - 1, width / 2, inner_radius - width);
	}

	std::vector<Eigen::Vector3f> positions;
	std::vector<uint32_t> indices;

	positions.emplace_back({ inner_radius + width, inner_radius, })




	mesh = Mesh::create();
	mesh->set_positions(positions, 0);
	mesh->set_normals(positions, 1);
	mesh->set_indices(indices);

	material = Material::create("standard_material");
	material->load_from_source("resources/shaders/standard_material.vs",
		"resources/shaders/standard_material.fs");
}

void PlanetRegion::render() const
{
	if (child)
		child->render();

	material->use();
	material->set_model_transform(Eigen::Affine3d::Identity());
	mesh->draw();
}
