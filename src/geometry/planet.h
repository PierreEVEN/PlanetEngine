#pragma once

#include "ui/ui.h"
#include "world/scene_component.h"

class Mesh;
class Material;
class PlanetRegion;

class Planet : public SceneComponent
{
	friend class PlanetRegion;
public:
	Planet(const World& world);

	static std::shared_ptr<Material> get_landscape_material();

	bool fragment_normals = false;

	float radius = 80000;
	int num_lods = 14;
	float cell_width = 1.0f;
	int cell_count = 10;
	bool double_sided = false;
	void regenerate();
	Eigen::Affine3d planet_global_transform;
	Eigen::Vector3f planet_color = Eigen::Vector3f(97.f / 256, 130.f / 256, 223.f / 256);
	Eigen::Affine3d planet_inverse_rotation;

protected:
	void tick(double delta_time) override;
	void render(Camera& camera) override;


private:
	std::shared_ptr<Mesh> root_mesh;
	std::shared_ptr<Mesh> child_mesh;
	std::shared_ptr<PlanetRegion> root;
	const World& world;
};

class PlanetRegion
{
public:
	PlanetRegion(const Planet& parent, const World& world, uint32_t lod_level, uint32_t my_level);

	void regenerate(int32_t cell_number, double width);

	void tick(double delta_time, int num_lods);
	void render(Camera& camera) const;

private:
	const Planet& planet;
	Eigen::Vector3d chunk_position;
	const World& world;
	double cell_size;
	int32_t cell_number;
	uint32_t current_lod;
	uint32_t num_lods;
	std::shared_ptr<PlanetRegion> child;

	Eigen::Affine3d lod_local_transform;
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
