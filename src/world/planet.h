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
	bool fragment_normals = false;

	float radius = 80000;
	int num_lods = 14;
	float cell_width = 1.0f;
	int cell_count = 10;
	bool double_sided = false;
	bool freeze_camera = true;
	void regenerate();
	Eigen::Affine3d planet_global_transform;
	Eigen::Affine3d planet_inverse_rotation;
	Eigen::Affine3d planet_orientation = Eigen::Affine3d::Identity();

	Eigen::Vector4f debug_vector = Eigen::Vector4f::Zero();

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
	PlanetRegion(Planet& parent, const World& world, uint32_t lod_level, uint32_t my_level);

	void regenerate(int32_t cell_number);

	void tick(double delta_time, int num_lods, double width);
	void render(Camera& camera);

	void rebuild_maps();

private:


	struct LandscapeChunkData
	{
		Eigen::Matrix4f Chunk_LocalTransform;
		Eigen::Matrix4f Chunk_PlanetTransform;
		float Chunk_PlanetRadius;
		float Chunk_CellWidth;
		int32_t Chunk_CellCount;
		int32_t Chunk_CurrentLOD;
		float test_rotation;

		bool operator==(const LandscapeChunkData& other) const
		{
			return Chunk_LocalTransform == other.Chunk_LocalTransform &&
				Chunk_PlanetTransform == other.Chunk_PlanetTransform &&
				Chunk_PlanetRadius == other.Chunk_PlanetRadius &&
				Chunk_CellWidth == other.Chunk_CellWidth &&
				Chunk_CellCount == other.Chunk_CellCount &&
				Chunk_CurrentLOD == other.Chunk_CurrentLOD;
		}
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

	Eigen::Affine3d lod_local_transform;
	std::shared_ptr<Texture2D> height_map;
	std::shared_ptr<Texture2D> normal_map;
};