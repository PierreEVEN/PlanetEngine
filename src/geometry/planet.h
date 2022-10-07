#pragma once

#include "ui/ui.h"
#include "world/scene_component.h"

class Mesh;
class Material;
class PlanetRegion;

class Planet : public SceneComponent
{
public:
	Planet(const World& world);

	static std::shared_ptr<Material> get_landscape_material();

	bool fragment_normals = false;

	float radius = 8000;
	bool morph_to_sphere = true;
protected:
	void tick(double delta_time) override;
	void render(Camera& camera) override;


private:
	std::shared_ptr<PlanetRegion> root;
	const World& world;
};

class PlanetRegion
{
public:
	PlanetRegion(const Planet& parent, const World& world, uint32_t lod_level, uint32_t my_level);

	void regenerate(int32_t cell_number, float width, double inner_radius);

	void tick(double delta_time);
	void render(Camera& camera) const;

private:
	const Planet& parent;
	Eigen::Vector3d chunk_position;
	const World& world;
	float cell_size;
	int32_t cell_number;
	uint32_t current_lod;
	uint32_t num_lods;
	Eigen::Affine3d transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<PlanetRegion> child;


	Eigen::Affine3d temp_location;
	Eigen::Affine3d temp_rotation;
	Eigen::Affine3d planet_rotation;
};

class PlanetInformations : public ImGuiWindow
{
public:
	PlanetInformations(Planet* in_planet) : planet(in_planet)
	{
		window_name = "planet editor";
	}
	void draw() override;

private:
	Planet* planet;
};