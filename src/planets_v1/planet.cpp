#include "planet.h"

#include <imgui.h>

#include "graphics/mesh.h"
#include "graphics/material.h"

Planet::Planet() : final_mesh(Mesh::create())
{
	planet_material = Material::create("standard_material");
	planet_material->load_from_source("resources/shaders/standard_material.vs",
	                                  "resources/shaders/standard_material.fs");

	constexpr double X = 0.525731112119133606;
	constexpr double Z = 0.850650808352039932;
	constexpr double N = 0.0;

	const auto points = std::vector<Eigen::Vector3d>{
		{-X, N, Z}, {X, N, Z}, {-X, N, -Z}, {X, N, -Z},
		{N, Z, X}, {N, Z, -X}, {N, -Z, X}, {N, -Z, -X},
		{Z, X, N}, {-Z, X, N}, {Z, -X, N}, {-Z, -X, N}
	};

	const auto triangles = std::vector<Eigen::Vector3i>{
		{0, 1, 4}, {0, 4, 9}, {9, 4, 5}, {4, 8, 5}, {4, 1, 8},
		{8, 1, 10}, {8, 10, 3}, {5, 8, 3}, {5, 2, 2}, {2, 3, 7},
		{7, 3, 10}, {7, 10, 6}, {7, 6, 11}, {11, 6, 0}, {0, 6, 1},
		{6, 10, 1}, {9, 11, 0}, {9, 2, 11}, {9, 5, 2}, {7, 11, 2}
	};

	for (const auto& tr : triangles)
		initial_regions.emplace_back(PlanetRegion(points[tr.x()], points[tr.y()], points[tr.z()]));

	regenerate();
}

Planet::~Planet()
{
	if (regeneration_thread.joinable())
		regeneration_thread.join();
}

std::atomic_uint32_t subdivision_progress;

void Planet::tick(double delta_time)
{
	SceneComponent::tick(delta_time);

	if (is_waiting_regeneration && regeneration_job_done)
	{
		final_mesh->rebuild_mesh_data();
		is_waiting_regeneration = false;
		regeneration_job_done = false;
	}

	if (ImGui::Begin("Planet manager"))
	{
		ImGui::SliderInt("subdivisions", &subdivisions, 0, 20);

		if (ImGui::Button("regenerate"))
			regenerate();

		if (subdivision_progress > 0)
			ImGui::Text("subdivision left : %d", static_cast<uint32_t>(subdivision_progress));
	}
	ImGui::End();
}

void Planet::render()
{
	SceneComponent::render();

	planet_material->use();
	planet_material->set_model_transform(get_world_transform());
	final_mesh->draw();
}

void Planet::regenerate()
{
	if (is_waiting_regeneration)
		return;

	is_waiting_regeneration = true;
	regeneration_job_done = false;

	if (regeneration_thread.joinable())
		regeneration_thread.join();

	subdivision_progress = 0;
	regeneration_thread = std::thread([&]
	{
		for (auto& region : initial_regions)
			region.subdivide(subdivisions);

		std::vector<Eigen::Vector3d> output_triangles;
		for (const auto& region : initial_regions)
			for (const auto& tr : region.collect_triangles())
				output_triangles.emplace_back(tr);


		std::vector<uint32_t> indices;
		std::vector<Eigen::Vector3f> positions;
		std::vector<Eigen::Vector3f> normals;
		uint32_t tr_index = 0;
		for (const auto& tri : output_triangles)
		{
			indices.emplace_back(tr_index++);
			positions.emplace_back(tri.cast<float>());
			normals.emplace_back(Eigen::Vector3f{1, 1, 0});
		}
		final_mesh->set_positions(positions, 0, true);
		final_mesh->set_normals(normals, 1, true);
		final_mesh->set_indices(indices, false);
		regeneration_job_done = true;
	});
}

PlanetRegion::PlanetRegion(const Eigen::Vector3d& in_a, const Eigen::Vector3d& in_b, const Eigen::Vector3d& in_c)
{
	a = in_a;
	b = in_b;
	c = in_c;
}

std::vector<Eigen::Vector3d> PlanetRegion::collect_triangles() const
{
	if (!children.empty())
	{
		std::vector<Eigen::Vector3d> output_triangles;
		for (const auto& child : children)
			for (const auto& tr : child.collect_triangles())
				output_triangles.emplace_back(tr);
		return output_triangles;
	}
	return {a, b, c};
}

void PlanetRegion::subdivide(int sub_index)
{
	subdivision_progress = sub_index;
	if (sub_index == 0)
		return;

	children.clear();

	children.emplace_back(PlanetRegion(a, (a + b) / 2, (c + a) / 2));
	children.emplace_back(PlanetRegion((a + b) / 2, b, (c + b) / 2));
	children.emplace_back(PlanetRegion((c + a) / 2, (c + b) / 2, c));
	children.emplace_back(PlanetRegion((c + a) / 2, (b + a) / 2, (c + b) / 2));

	for (auto& child : children)
		child.subdivide(sub_index - 1);
}
