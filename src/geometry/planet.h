#pragma once

#include <thread>

#include "world/scene_component.h"

class Mesh;
class Material;
class PlanetRegion;

class Planet : public SceneComponent
{
public:
	Planet(const World& world);

	static std::shared_ptr<Material> get_landscape_material();

protected:
	void tick(double delta_time) override;
	void render() override;

private:
	std::shared_ptr<PlanetRegion> root;
	const World& world;
};

class PlanetRegion
{
public:
	PlanetRegion(const World& world, uint32_t lod_level);

	void regenerate(uint32_t cell_number, double width, double inner_radius, const Eigen::Vector3d& position);

	void tick(double delta_time);
	void render();

	Eigen::Vector3d position;
private:

	const World& world;
	double cell_size;
	int lod_level;
	Eigen::Affine3d transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<PlanetRegion> child;
};
