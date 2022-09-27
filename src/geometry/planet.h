#pragma once

#include <thread>

#include "world/scene_component.h"

class Mesh;
class Material;
class PlanetRegion;

enum class RegionOrientation
{
	NE,
	SE,
	SO,
	NO
};

class Planet : public SceneComponent
{
public:
	Planet(const World& world);
	virtual ~Planet() override;

protected:
	void tick(double delta_time) override;
	void render() override;

private:
	std::shared_ptr<PlanetRegion> root;
	const World& world;
	Eigen::Matrix4f world_target_matrix  = Eigen::Matrix4f::Identity();
};

class PlanetRegion
{
public:
	PlanetRegion(int subdivision_level, double width, double inner_radius, const Eigen::Vector3d& position, RegionOrientation orientation);

	void render();

private:

	Eigen::Vector3d position;
	int subdivision_level;
	Eigen::Affine3d transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	std::shared_ptr<PlanetRegion> child;
};
