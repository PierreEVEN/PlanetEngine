#pragma once

#include <Eigen/Dense>
#include <memory>

class ComputeShader;
class Material;
class Texture2D;
class Planet;
class Camera;
class World;

class PlanetChunk {
public:
    PlanetChunk(Planet& parent, const World& world, uint32_t lod_level, uint32_t my_level);

    void regenerate(int32_t cell_number);
    void force_rebuild_maps();
    void rebuild_maps();
    void tick(double delta_time, int num_lods, double width);
    void render(Camera& camera);


private:
    struct LandscapeChunkData {
        Eigen::Matrix4f Chunk_LocalTransform;
        Eigen::Matrix4f Chunk_PlanetModel;
        Eigen::Matrix4f Chunk_LocalOrientation;
        float           Chunk_PlanetRadius;
        float           Chunk_CellWidth;
        int32_t         Chunk_CellCount;
        int32_t         Chunk_CurrentLOD;

        bool operator==(const LandscapeChunkData& other) const;
    };

    LandscapeChunkData last_chunk_data;

    Planet&                      planet;
    Eigen::Vector3d              chunk_position;
    const World&                 world;
    double                       cell_size;
    int32_t                      cell_number;
    uint32_t                     current_lod;
    uint32_t                     num_lods;
    std::shared_ptr<PlanetChunk> child;
    bool                         force_rebuild = true;

    Eigen::Affine3d            mesh_transform_cs;
    std::shared_ptr<Texture2D> chunk_height_map;
    std::shared_ptr<Texture2D> chunk_normal_map;
};
