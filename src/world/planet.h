#pragma once

#include "world/scene_component.h"

class Texture2D;
class ComputeShader;
class Mesh;
class Material;
class PlanetChunk;
class World;

class Planet : public SceneComponent {
    friend class PlanetChunk;
public:
    static std::shared_ptr<Planet> create(const std::string& name, const std::shared_ptr<SceneComponent>& player);

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

    virtual Class get_class() override { return {this}; }

    struct AtmosphereSettings {
        float           atmosphere_depth     = 200000;
        float           density_falloff      = 6;
        float           scatter_strength     = 2;
        Eigen::Vector4f scatter_coefficients = Eigen::Vector4f(700.f, 550.f, 460.f, 4.f);
        float           epsilon              = 1;
    };

    bool               enable_atmosphere   = true;
    AtmosphereSettings atmosphere_settings = {};

protected:
    void tick(double delta_time) override;
    void render(Camera& camera) override;

private:
    Planet(const std::string& name, const std::shared_ptr<SceneComponent>& player);

    std::shared_ptr<SceneComponent> player;
    std::shared_ptr<Mesh>           root_mesh;
    std::shared_ptr<Mesh>           child_mesh;
    std::shared_ptr<PlanetChunk>    root;

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
    bool            freeze_camera   = false;
    bool            freeze_updates  = false;
    bool            double_sided    = false;
    bool            display_normals = false;
    bool            dirty           = true;
    Eigen::Vector4f debug_vector    = Eigen::Vector4f::Zero();

    void rebuild_mesh();

    // Transformations
    Eigen::Affine3d    mesh_transform_ws    = Eigen::Affine3d::Identity();
    Eigen::Quaterniond mesh_rotation_ws     = Eigen::Quaterniond::Identity();
    Eigen::Quaterniond mesh_rotation_ps     = Eigen::Quaterniond::Identity();
    Eigen::Quaterniond inv_mesh_rotation_ws = Eigen::Quaterniond::Identity();

    // GPU Objects
    std::shared_ptr<Material>      landscape_material            = nullptr;
    std::shared_ptr<Material>      landscape_water_material      = nullptr;
    std::shared_ptr<Material>      debug_normal_display_material = nullptr;
    std::shared_ptr<ComputeShader> compute_positions             = nullptr;
    std::shared_ptr<ComputeShader> compute_normals               = nullptr;
    std::shared_ptr<ComputeShader> compute_fix_seams             = nullptr;
    std::shared_ptr<Texture2D>     grass_albedo                  = nullptr;
    std::shared_ptr<Texture2D>     grass_normal                  = nullptr;
    std::shared_ptr<Texture2D>     grass_mrao                    = nullptr;
    std::shared_ptr<Texture2D>     rock_albedo                   = nullptr;
    std::shared_ptr<Texture2D>     rock_normal                   = nullptr;
    std::shared_ptr<Texture2D>     rock_mrao                     = nullptr;
    std::shared_ptr<Texture2D>     sand_albedo                   = nullptr;
    std::shared_ptr<Texture2D>     sand_normal                   = nullptr;
    std::shared_ptr<Texture2D>     sand_mrao                     = nullptr;
    std::shared_ptr<Texture2D>     water_normal                  = nullptr;
    std::shared_ptr<Texture2D>     water_displacement            = nullptr;
};
