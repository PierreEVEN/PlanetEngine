#pragma once

#include <thread>

#include "world/scene_component.h"

class Mesh;
class Material;
class PlanetRegion;

class Planet : public SceneComponent
{
public:

	Planet();
	virtual ~Planet() override;

protected:
	void tick(double delta_time) override;
	void render() override;

private:
	std::vector<PlanetRegion> initial_regions;

	std::shared_ptr<Mesh> final_mesh;
	std::shared_ptr<Material> planet_material;

	void regenerate();

	int subdivisions = 1;
	std::atomic_bool is_waiting_regeneration = false;
	std::atomic_bool regeneration_job_done = false;
	std::thread regeneration_thread;
};

class PlanetRegion
{
public:
	PlanetRegion(const Eigen::Vector3d& a, const Eigen::Vector3d& b, const Eigen::Vector3d& c);

	[[nodiscard]] std::vector<Eigen::Vector3d> collect_triangles() const;

	void subdivide(int sub_index);

private:

	PlanetRegion* parent = nullptr;
	std::vector<PlanetRegion*> neighbors;
	std::vector<PlanetRegion> children;

	Eigen::Vector3d a, b, c;
};
