#include <imgui.h>
#include <ImGuizmo.h>

#include "camera.h"
#include "Planet.h"

#include "graphics/mesh.h"
#include "graphics/material.h"

Planet::Planet(const World& in_world) : world(in_world)
{
	root = std::make_shared<PlanetRegion>(1, 130, 200, Eigen::Vector3d(0, 0, 0), RegionOrientation::NE);
}

Planet::~Planet()
{
}

Eigen::Vector3f world_origin;

void Planet::tick(double delta_time)
{
	SceneComponent::tick(delta_time);

	// Draw world gizmo
	ImGuizmo::RecomposeMatrixFromComponents(world_origin.data(), Eigen::Vector3f(0, 0, 0).data(),
	                                        Eigen::Vector3f(1, 1, 1).data(), world_target_matrix.data());
	ImGuizmo::Enable(true);
	Manipulate(
		Eigen::Matrix4f(world.get_camera()->view_matrix().cast<float>()).data(),
		Eigen::Matrix4f(world.get_camera()->projection_matrix().cast<float>()).data(),
		ImGuizmo::TRANSLATE,
		ImGuizmo::WORLD,
		world_target_matrix.data(),
		nullptr,
		nullptr);
	ImGuizmo::DecomposeMatrixToComponents(world_target_matrix.data(), world_origin.data(),
	                                      Eigen::Vector3f(0, 0, 0).data(), Eigen::Vector3f(1, 1, 1).data());

	ImGui::DragFloat2("world origin", world_origin.data());
}

void Planet::render()
{
	SceneComponent::render();

	root->render();
}

PlanetRegion::PlanetRegion(int in_subdivision_level, double width, double inner_radius, const Eigen::Vector3d& in_position,
                           RegionOrientation orientation) : position(in_position), subdivision_level(in_subdivision_level)
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

	for (int i = 0; i < 11; ++i)
	{
		normals[i] = Eigen::Vector3f(
			static_cast<float>(fmod(std::abs(sin((subdivision_level + 1) * 3678.45678)), 1.0)),
			static_cast<float>(fmod(std::abs(sin((subdivision_level + 1) * 98.0987654)), 1.0)),
			static_cast<float>(fmod(std::abs(sin((subdivision_level + 1) * 567.845496)), 1.0)));
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

void PlanetRegion::render()
{
	transform = Eigen::Affine3d::Identity();
	if (world_origin.x() >= position.x() && world_origin.y() < position.y())
		transform.rotate(Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ()));
	else if (world_origin.x() < position.x() && world_origin.y() >= position.y())
		transform.rotate(Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitZ()));
	else if (world_origin.x() < position.x() && world_origin.y() < position.y())
		transform.rotate(Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ()));
	else if (world_origin.x() >= position.x() && world_origin.y() >= position.y())
		transform.rotate(Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()));

	std::cout << "begin level " << subdivision_level << std::endl;
	std::cout << transform.matrix() << std::endl;

	if (child)
		child->render();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	material->use();
	material->set_model_transform(transform);
	mesh->draw();
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glFrontFace(GL_CW);
	mesh->draw();
	glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
}
