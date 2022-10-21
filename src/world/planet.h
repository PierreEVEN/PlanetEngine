#pragma once

#include "ui/ui.h"
#include "world/scene_component.h"

class Texture2D;
class Mesh;
class Material;
class PlanetRegion;

class Planet : public SceneComponent
{
	friend class PlanetRegion;
public:
	Planet(const std::string& name);
	static std::shared_ptr<Material> get_landscape_material();
	void draw_ui() override;

	[[nodiscard]] float get_radius() const { return radius;  }

	void set_radius(float in_radius)
	{
		radius = in_radius;
		dirty = true;
	}

	void set_max_lods(int in_max_lods)
	{
		num_lods = in_max_lods;
		dirty = true;
	}

	void set_cell_count(int in_cell_count)
	{
		cell_count = in_cell_count;
		dirty = true;
	}

protected:
	void tick(double delta_time) override;
	void render(Camera& camera) override;

private:
	std::shared_ptr<Mesh> root_mesh;
	std::shared_ptr<Mesh> child_mesh;
	std::shared_ptr<PlanetRegion> root;
	const World& world;

	// Parameters
	float radius = 80000;
	int num_lods = 14;
	float cell_width = 1.0f;
	int cell_count = 10;

	// debug
	bool freeze_camera = false;
	bool freeze_updates = false;
	bool double_sided = false;
	bool dirty = true;
	void regenerate();
	Eigen::Affine3d planet_global_transform = Eigen::Affine3d::Identity();
	Eigen::Affine3d world_orientation = Eigen::Affine3d::Identity();
	Eigen::Affine3d local_orientation = Eigen::Affine3d::Identity();
	Eigen::Affine3d planet_inverse_rotation = Eigen::Affine3d::Identity();
	Eigen::Vector4f debug_vector = Eigen::Vector4f::Zero();
};

class PlanetRegion
{
public:
	PlanetRegion(Planet& parent, const World& world, uint32_t lod_level, uint32_t my_level);

	void regenerate(int32_t cell_number);

	void tick(double delta_time, int num_lods, double width);
	void render(Camera& camera);

	void rebuild_maps();

	void force_rebuild_maps();

private:
	struct LandscapeChunkData
	{
		Eigen::Matrix4f Chunk_LocalTransform;
		Eigen::Matrix4f Chunk_PlanetModel;
		Eigen::Matrix4f Chunk_LocalOrientation;
		float Chunk_PlanetRadius;
		float Chunk_CellWidth;
		int32_t Chunk_CellCount;
		int32_t Chunk_CurrentLOD;

		bool operator==(const LandscapeChunkData& other) const;
	};

	LandscapeChunkData last_chunk_data;

	Planet& planet;
	Eigen::Vector3d chunk_position;
	const World& world;
	double cell_size;
	int32_t cell_number;
	uint32_t current_lod;
	uint32_t num_lods;
	std::shared_ptr<PlanetRegion> child;
	bool force_rebuild = true;

	Eigen::Affine3d lod_local_transform;
	std::shared_ptr<Texture2D> chunk_height_map;
	std::shared_ptr<Texture2D> chunk_normal_map;
};
