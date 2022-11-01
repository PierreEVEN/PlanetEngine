#include <GL/gl3w.h>

#include "graphics/camera.h"
#include "planet.h"

#include "planet_chunk.h"
#include "world.h"

#include <imgui.h>

#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/compute_shader.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/texture_image.h"
#include "ui/widgets.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"

Planet::Planet(const std::string& name)
    : SceneComponent(name), world(Engine::get().get_world()) {
    root  = std::make_shared<PlanetChunk>(*this, world, 16, 0);
    dirty = true;

    compute_positions = ComputeShader::create("Planet compute position", "resources/shaders/compute/planet_compute_position.cs");
    compute_fix_seams = ComputeShader::create("Planet Fix Seams", "resources/shaders/compute/planet_fix_seams.cs");
    compute_normals   = ComputeShader::create("Planet compute normals", "resources/shaders/compute/planet_compute_normals.cs");

    compute_positions->on_reload.add_object(root.get(), &PlanetChunk::force_rebuild_maps);
    compute_fix_seams->on_reload.add_object(root.get(), &PlanetChunk::force_rebuild_maps);
    compute_normals->on_reload.add_object(root.get(), &PlanetChunk::force_rebuild_maps);

    landscape_material            = Material::create("planet material", "resources/shaders/planet_material.vs", "resources/shaders/planet_material.fs");
    debug_normal_display_material = Material::create("planet material normals", "resources/shaders/planet_material.vs", "resources/shaders/planet_material_normal_display.fs",
                                                     "resources/shaders/planet_material_normal_display.gs");
}

void Planet::draw_ui() {
    SceneComponent::draw_ui();
    ImGui::SliderInt("num LODs : ", &num_lods, 1, 40);
    ImGui::DragFloat("radius : ", &radius, 10);
    if (ImGui::SliderInt("cell number", &cell_count, 1, 120) ||
        ImGui::SliderFloat("cell_width : ", &cell_width, 0.05f, 10))
        dirty = true;

    ImGui::DragFloat("rotation speed : ", &rotation_speed, 0.01f);
    ImGui::DragFloat("Orbit speed : ", &orbit_speed, 0.01f);
    ImGui::DragFloat("Orbit distance : ", &orbit_distance, 100);

    ImGui::Separator();
    ImGui::Checkbox("Show normals", &display_normals);
    ImGui::Checkbox("Double sided", &double_sided);
    ImGui::Checkbox("Freeze Camera", &freeze_camera);
    ImGui::Checkbox("Freeze Updates", &freeze_updates);
    ImGui::DragFloat4("debug vector", debug_vector.data());

    Eigen::Quaterniond rot = Eigen::Quaterniond(world_orientation.rotation());

    if (ui::rotation_edit(rot, "camera rotation")) {
        world_orientation = Eigen::Affine3d::Identity();
        world_orientation.rotate(rot);
    }
}

static void generate_rectangle_area(std::vector<uint32_t>& indices, std::vector<Eigen::Vector3f>& positions,
                                    int32_t                x_min, int32_t                         x_max, int32_t z_min, int32_t z_max,
                                    int32_t                cell_min, int32_t                      cell_max) {
    // Requirement for Y val
    const float distance_max = static_cast<float>(cell_max);
    float       min_global   = static_cast<float>(cell_min);

    // Handle LOD 0 case
    if (min_global == distance_max)
        min_global = 0;

    const auto current_index_offset = static_cast<uint32_t>(positions.size());
    for (int32_t z = z_min; z <= z_max; ++z)
        for (int32_t x = x_min; x <= x_max; ++x) {
            // The Y val is used to store the progression from previous LOD to the next one
            const float Linf_distance = static_cast<float>(std::max(std::abs(x), std::abs(z))); // Tchebychev distance
            const float y_val_weight  = (Linf_distance - min_global) / (distance_max - min_global);
            const bool  x_aligned     = std::abs(x) > std::abs(z);
            const int   mask          = std::abs(x) % 2 == 0 && !x_aligned || std::abs(z) % 2 == 0 && x_aligned;
            positions.emplace_back(Eigen::Vector3f(static_cast<float>(x), mask * y_val_weight, static_cast<float>(z)));
        }

    const uint32_t x_width = std::abs(x_max - x_min);
    const uint32_t z_width = std::abs(z_max - z_min);
    for (uint32_t z = 0; z < z_width; ++z)
        for (uint32_t x = 0; x < x_width; ++x) {
            uint32_t base_index = x + z * (x_width + 1) + current_index_offset;
            if (positions[base_index].x() * positions[base_index].z() > 0) {
                indices.emplace_back(base_index);
                indices.emplace_back(base_index + x_width + 2);
                indices.emplace_back(base_index + x_width + 1);
                indices.emplace_back(base_index);
                indices.emplace_back(base_index + 1);
                indices.emplace_back(base_index + x_width + 2);
            } else {
                indices.emplace_back(base_index);
                indices.emplace_back(base_index + 1);
                indices.emplace_back(base_index + x_width + 1);
                indices.emplace_back(base_index + 1);
                indices.emplace_back(base_index + x_width + 2);
                indices.emplace_back(base_index + x_width + 1);
            }
        }
}

void Planet::regenerate() {
    GL_CHECK_ERROR();
    STAT_ACTION("Generate planet mesh : [" + name + "]");
    STAT_FRAME("regenerate planet mesh");
    std::vector<Eigen::Vector3f> positions_root;
    std::vector<uint32_t>        indices_root;

    {
        STAT_ACTION("regenerate root mesh");
        generate_rectangle_area(indices_root, positions_root,
                                -cell_count * 2 - 1,
                                cell_count * 2 + 1,
                                -cell_count * 2 - 1,
                                cell_count * 2 + 1,
                                0, cell_count * 2);

        root_mesh = Mesh::create("planet root mesh");
        root_mesh->set_positions(positions_root, 0, true);
        root_mesh->set_indices(indices_root);
    }

    GL_CHECK_ERROR();

    std::vector<Eigen::Vector3f> positions_child;
    std::vector<uint32_t>        indices_child;

    {
        STAT_ACTION("Generate planet mesh vertices : [" + name + "]");
        // TOP side (larger)
        generate_rectangle_area(indices_child, positions_child,
                                cell_count,
                                cell_count * 2 + 1,
                                -cell_count - 1,
                                cell_count * 2 + 1,
                                cell_count, cell_count * 2 + 1);

        // RIGHT side (larger)
        generate_rectangle_area(indices_child, positions_child,
                                -cell_count * 2 - 1,
                                cell_count,
                                cell_count,
                                cell_count * 2 + 1,
                                cell_count, cell_count * 2 + 1);

        // BOTTOM side
        generate_rectangle_area(indices_child, positions_child,
                                -cell_count * 2 - 1,
                                -cell_count - 1,
                                -cell_count * 2 - 1,
                                cell_count,
                                cell_count + 1, cell_count * 2 + 1);

        // LEFT side
        generate_rectangle_area(indices_child, positions_child,
                                -cell_count - 1,
                                cell_count * 2 + 1,
                                -cell_count * 2 - 1,
                                -cell_count - 1,
                                cell_count + 1, cell_count * 2 + 1);
    }
    child_mesh = Mesh::create("planet child mesh");
    child_mesh->set_positions(positions_child, 0, true);
    child_mesh->set_indices(indices_child);

    GL_CHECK_ERROR();

    STAT_ACTION("regenerate planet children chunk : [" + name + "] ");
    root->regenerate(cell_count);
    GL_CHECK_ERROR();
    dirty = false;
}

static Eigen::Quaterniond closest_rotation_to(const Eigen::Quaterniond& from, const Eigen::Vector3d& forward, double min_delta) {
    const Eigen::Vector3d    current_front = from * Eigen::Vector3d(1, 0, 0);
    const Eigen::Quaterniond delta         = Eigen::Quaterniond::FromTwoVectors(current_front, forward);

    const auto forward_step = delta * Eigen::Vector3d(0, 0, 1);
    const auto left_step    = delta * Eigen::Vector3d(0, 1, 0);

    const double forward_angle = acos(forward_step.dot(Eigen::Vector3d(0, 0, 1)));
    const double right_angle   = acos(left_step.dot(Eigen::Vector3d(0, 1, 0)));

    if (forward_angle > min_delta || right_angle > min_delta)
        return delta * from;

    return from;
}


void Planet::tick(double delta_time) {
    STAT_FRAME("Planet_Tick");
    SceneComponent::tick(delta_time);

    current_orbit += orbit_speed * delta_time;
    current_rotation += rotation_speed * delta_time;
    set_local_position(Eigen::Vector3d(std::cos(current_orbit), std::sin(current_orbit), 0) * orbit_distance);
    set_local_rotation(Eigen::Quaterniond(Eigen::AngleAxisd(current_rotation, Eigen::Vector3d::UnitZ())));

    if (dirty) {
        regenerate();
    }

    {
        STAT_FRAME("compute planet global transform");

        if (!freeze_camera) {
            // Get camera direction from planet center
            const auto camera_direction = get_world_rotation().inverse() * (Engine::get().get_world().get_camera()->get_world_position() - get_world_position()).normalized();

            // Compute global rotation snapping step
            const double max_cell_radian_step = cell_width * std::pow(2, num_lods) / (radius * 2);
            rotation_to_camera                = closest_rotation_to(rotation_to_camera, camera_direction, max_cell_radian_step);
            local_orientation                 = Eigen::Affine3d::Identity();
            local_orientation.rotate(rotation_to_camera);

            world_orientation = get_world_rotation() * local_orientation;
        }

        planet_inverse_rotation = world_orientation.inverse();
        // Compute global planet transformation (ensure ground is always close to origin)
        planet_global_transform = Eigen::Affine3d::Identity();
        planet_global_transform.translate(get_world_position() - Engine::get().get_world().get_camera()->get_world_position());
        planet_global_transform = planet_global_transform * world_orientation;
        planet_global_transform.translate(Eigen::Vector3d(radius, 0, 0));
    }

    const double camera_distance_to_ground = (Engine::get().get_world().get_camera()->
                                                            get_world_position() -
                                              get_world_position()).norm() - static_cast<double>(radius);

    const double normalized_distance = std::max(1.0, camera_distance_to_ground / (cell_width * (cell_count * 4 + 2)));
    const int    min_lod             = std::min(num_lods - 1, static_cast<int>(std::log2(normalized_distance)));
    const int    max_lod             = num_lods;
    const float  initial_cell_width  = cell_width * static_cast<float>(std::pow(2, min_lod));

    root->tick(delta_time, max_lod - min_lod, initial_cell_width);
}


static std::shared_ptr<Texture2D> grass_albedo       = nullptr;
static std::shared_ptr<Texture2D> grass_normal       = nullptr;
static std::shared_ptr<Texture2D> grass_mrao         = nullptr;
static std::shared_ptr<Texture2D> rock_albedo        = nullptr;
static std::shared_ptr<Texture2D> rock_normal        = nullptr;
static std::shared_ptr<Texture2D> rock_mrao          = nullptr;
static std::shared_ptr<Texture2D> sand_albedo        = nullptr;
static std::shared_ptr<Texture2D> sand_normal        = nullptr;
static std::shared_ptr<Texture2D> sand_mrao          = nullptr;
static std::shared_ptr<Texture2D> water_normal       = nullptr;
static std::shared_ptr<Texture2D> water_displacement = nullptr;

static void load_textures() {
    if (!grass_albedo) {
        grass_albedo = Texture2D::create("terrain grass albedo", "resources/textures/terrain/wispy-grass-meadow_albedo.png", {.srgb = true});
        grass_normal = Texture2D::create("terrain grass normal", "resources/textures/terrain/wispy-grass-meadow_normal-dx.png");
        grass_mrao   = Texture2D::create("terrain grass mrao", "resources/textures/terrain/wispy-grass-meadow_mrao.jpg");

        rock_albedo = Texture2D::create("terrain rock albedo", "resources/textures/terrain/pine_forest_ground1_albedo.png", {.srgb = true});
        rock_normal = Texture2D::create("terrain rock normal", "resources/textures/terrain/pine_forest_ground1_Normal-dx.png");
        rock_mrao   = Texture2D::create("terrain rock mrao", "resources/textures/terrain/pine_forest_ground1_mrao.jpg");

        sand_albedo = Texture2D::create("terrain sand albedo", "resources/textures/terrain/wavy-sand_albedo.png", {.srgb = true});
        sand_normal = Texture2D::create("terrain sand normal", "resources/textures/terrain/wavy-sand_normal-dx.png");
        sand_mrao   = Texture2D::create("terrain sand mrao", "resources/textures/terrain/wavy-sand_mrao.jpg");

        water_normal       = Texture2D::create("water normal", "resources/textures/water/water_normal.png");
        water_displacement = Texture2D::create("water displacement", "resources/textures/water/water_distortion.png");
    }
}


void Planet::render(Camera& camera) {
    STAT_FRAME("Render Planet");
    SceneComponent::render(camera);

    if (landscape_material->bind()) {
        landscape_material->set_model_transform(planet_global_transform);
        glUniform1f(landscape_material->binding("radius"), radius);
        glUniform1f(landscape_material->binding("grid_cell_count"), static_cast<float>(cell_count));
        glUniform4fv(landscape_material->binding("debug_vector"), 1, debug_vector.data());
        glUniformMatrix4fv(landscape_material->binding("planet_world_orientation"), 1, false, local_orientation.cast<float>().matrix().data());

        glUniformMatrix4fv(landscape_material->binding("inv_planet_world_orientation"), 1, false, local_orientation.cast<float>().inverse().matrix().data());
        glUniformMatrix4fv(landscape_material->binding("inv_model"), 1, false, planet_global_transform.cast<float>().inverse().matrix().data());

        glUniformMatrix3fv(landscape_material->binding("scene_rotation"), 1, false, get_world_rotation().cast<float>().matrix().data());
        glUniformMatrix3fv(landscape_material->binding("inv_scene_rotation"), 1, false, get_world_rotation().inverse().cast<float>().matrix().data());

        // Bind textures
        load_textures();
        landscape_material->bind_texture(grass_albedo, "grass_color");
        landscape_material->bind_texture(rock_albedo, "rock_color");
        landscape_material->bind_texture(sand_albedo, "sand_color");
        landscape_material->bind_texture(grass_normal, "grass_normal");
        landscape_material->bind_texture(rock_normal, "rock_normal");
        landscape_material->bind_texture(sand_normal, "sand_normal");
        landscape_material->bind_texture(grass_mrao, "grass_mrao");
        landscape_material->bind_texture(rock_mrao, "rock_mrao");
        landscape_material->bind_texture(sand_mrao, "sand_mrao");
        landscape_material->bind_texture(water_normal, "water_normal");
        landscape_material->bind_texture(water_displacement, "water_displacement");

        root->render(camera);
    }

    if (display_normals && debug_normal_display_material->bind()) {

        root->render(camera);
    }
}
