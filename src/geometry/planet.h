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
};

class PlanetRegion
{
public:
	PlanetRegion(int subdivision_level, double width, double inner_radius);

	void render() const;

private:

	int subdivision_level;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	std::shared_ptr<PlanetRegion> child;
};
