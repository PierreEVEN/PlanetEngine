#pragma once

#include "world/scene_component.h"

class ComputeShader;
class Mesh;
class Material;
class PlanetChunk;

class Planet : public SceneComponent {
    friend class PlanetChunk;
public:
    Planet(const std::string& name);
    void draw_ui() override;

    [[nodiscard]] float get_radius() const { return radius; }

    void set_radius(float in_radius) {
        radius = in_radius;
        dirty  = true;
    }

    void set_max_lods(int in_max_lods) {
        num_lods = in_max_lods;
        dirty    = true;
    }

    void set_cell_count(int in_cell_count) {
        cell_count = in_cell_count;
        dirty      = true;
    }

    void set_orbit_distance(float distance) {
        orbit_distance = distance;
    }

    void set_rotation_speed(float speed) {
        rotation_speed = speed;
    }

    void set_orbit_speed(float speed) {
        orbit_speed = speed;
    }

    virtual Class get_class() override { return Class(this); }
protected:
    void tick(double delta_time) override;
    void render(Camera& camera) override;

private:
    std::shared_ptr<Mesh>        root_mesh;
    std::shared_ptr<Mesh>        child_mesh;
    std::shared_ptr<PlanetChunk> root;
    const World&                 world;

    // Parameters
    float radius     = 80000;
    int   num_lods   = 14;
    float cell_width = 1.0f;
    int   cell_count = 10;

    // Orbit settings
    float  rotation_speed   = 0.001f;
    float  orbit_distance   = 0;
    float  orbit_speed      = 0;
    double current_rotation = 0;
    double current_orbit    = 0;

    // debug
    bool                           freeze_camera   = false;
    bool                           freeze_updates  = false;
    bool                           double_sided    = false;
    bool                           display_normals = false;
    bool                           dirty           = true;
    void                           regenerate();
    Eigen::Affine3d                planet_global_transform = Eigen::Affine3d::Identity();
    Eigen::Affine3d                world_orientation       = Eigen::Affine3d::Identity();
    Eigen::Affine3d                local_orientation       = Eigen::Affine3d::Identity();
    Eigen::Quaterniond             rotation_to_camera      = Eigen::Quaterniond::Identity();
    Eigen::Affine3d                planet_inverse_rotation = Eigen::Affine3d::Identity();
    Eigen::Vector4f                debug_vector            = Eigen::Vector4f::Zero();
    std::shared_ptr<Material>      landscape_material;
    std::shared_ptr<Material>      debug_normal_display_material;
    std::shared_ptr<ComputeShader> compute_positions = nullptr;
    std::shared_ptr<ComputeShader> compute_normals   = nullptr;
    std::shared_ptr<ComputeShader> compute_fix_seams         = nullptr;
};
