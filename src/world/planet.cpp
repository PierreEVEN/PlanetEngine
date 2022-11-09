#include <GL/gl3w.h>

#include "graphics/camera.h"
#include "planet.h"

#include "planet_chunk.h"

#include <imgui.h>

#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/compute_shader.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/texture_image.h"
#include "ui/widgets.h"
#include "utils/gl_tools.h"
#include "utils/maths.h"
#include "utils/profiler.h"

Planet::Planet(const std::string& name, const std::shared_ptr<SceneComponent>& in_player)
    : SceneComponent(name), player(in_player) {
    root  = std::make_shared<PlanetChunk>(*this, 16, 0);
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


std::shared_ptr<Planet> Planet::create(const std::string& name, const std::shared_ptr<SceneComponent>& player) {
    return std::shared_ptr<Planet>(new Planet(name, player));
}
void Planet::draw_ui() {
    SceneComponent::draw_ui();
    ImGui::Text("Mesh");
    ImGui::SliderInt("num LODs : ", &num_lods, 1, 40);
    ImGui::DragFloat("radius : ", &radius, 10);
    if (ImGui::SliderInt("cell number", &cell_count, 1, 120) ||
        ImGui::SliderFloat("cell_width : ", &cell_width, 0.05f, 10))
        dirty = true;

    ImGui::Separator();
    ImGui::Text("Atmosphere");
    ImGui::Checkbox("enable", &enable_atmosphere);
    if (enable_atmosphere) {
        ImGui::DragFloat("Depth", &atmosphere_settings.density_falloff, 100);
        ImGui::SliderFloat("Density Falloff", &atmosphere_settings.density_falloff, 0.01f, 12);
        ImGui::SliderFloat("Scatter strength", &atmosphere_settings.scatter_strength, 0.01f, 10);

        ImGui::SliderFloat3("Scatter coefficients", atmosphere_settings.scatter_coefficients.data(), 0, 2000);
        ImGui::SliderFloat("Scatter power", &atmosphere_settings.scatter_coefficients.w(), 0.01f, 10);
    }
    ImGui::Separator();
    ImGui::Text("Transformations");
    ImGui::DragFloat("rotation speed : ", &rotation_speed, 0.01f);
    ImGui::DragFloat("Orbit speed : ", &orbit_speed, 0.01f);
    ImGui::DragFloat("Orbit distance : ", &orbit_distance, 100);

    ImGui::Separator();
    ImGui::Text("Debug");
    ImGui::Checkbox("Show normals", &display_normals);
    ImGui::Checkbox("Double sided", &double_sided);
    ImGui::Checkbox("Freeze Camera", &freeze_camera);
    ImGui::Checkbox("Freeze Updates", &freeze_updates);
    ImGui::DragFloat4("debug vector", debug_vector.data());
    ui::rotation_edit(mesh_rotation_ws, "camera rotation");
}

static void generate_rectangle_area(std::vector<uint32_t>& indices, std::vector<Eigen::Vector3f>& positions,
                                    int32_t                x_min, int32_t                         x_max,
                                    int32_t                z_min, int32_t                         z_max,
                                    int32_t                cell_min, int32_t                      cell_max) {
    // Requirement for Y val
    const auto distance_max = static_cast<float>(cell_max);
    auto       min_global   = static_cast<float>(cell_min);

    // Handle LOD 0 case
    if (abs(min_global - distance_max) < 0.0001)
        min_global = 0;

    const auto current_index_offset = static_cast<uint32_t>(positions.size());
    for (int32_t z = z_min; z <= z_max; ++z)
        for (int32_t x = x_min; x <= x_max; ++x) {
            // The Y val is used to store the progression from previous LOD to the next one
            const float linf_distance = static_cast<float>(std::max(std::abs(x), std::abs(z))); // Tchebychev distance
            const float y_val_weight  = (linf_distance - min_global) / (distance_max - min_global);
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

void Planet::rebuild_mesh() {
    GL_CHECK_ERROR();
    STAT_ACTION("Generate planet mesh : [" + name + "]");
    STAT_FRAME("rebuild_mesh planet mesh");

    {
        STAT_ACTION("rebuild_mesh root mesh");
        std::vector<uint32_t>        indices_root;
        std::vector<Eigen::Vector3f> positions_root;
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

    {
        STAT_ACTION("Generate planet mesh vertices : [" + name + "]");
        std::vector<uint32_t>        indices_child;
        std::vector<Eigen::Vector3f> positions_child;
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
                                -cell_count - 1, cell_count + 1, cell_count * 2 + 1);
        child_mesh = Mesh::create("planet child mesh");
        child_mesh->set_positions(positions_child, 0, true);
        child_mesh->set_indices(indices_child);
    }

    GL_CHECK_ERROR();

    STAT_ACTION("rebuild_mesh planet children chunk : [" + name + "] ");
    root->regenerate(cell_count);
    GL_CHECK_ERROR();
    dirty = false;
}

void Planet::tick(double delta_time) {
    STAT_FRAME("Planet_Tick");
    SceneComponent::tick(delta_time);

    if (dirty) {
        rebuild_mesh();
    }

    {
        STAT_FRAME("compute planet global transform");

        if (!freeze_camera) {
            // Get camera direction from planet center
            const auto camera_direction_ps = get_world_rotation().inverse() * (player->get_world_position() - get_world_position()).normalized();

            // Compute global rotation snapping step
            const double max_cell_radian_step = cell_width * std::pow(2, num_lods) / (radius * 2.0);
            mesh_rotation_ps                  = Maths::closest_rotation_to(mesh_rotation_ps, camera_direction_ps, max_cell_radian_step);

            // Mesh rotation in world space
            mesh_rotation_ws = get_world_rotation() * mesh_rotation_ps;
        }

        // Transform world space to mesh space
        inv_mesh_rotation_ws = mesh_rotation_ws.inverse();

        // Mesh transform in world space
        mesh_transform_ws = Eigen::Affine3d::Identity();
        // 1. Add camera offset
        mesh_transform_ws.translate(get_world_position() - player->get_world_position());
        // 2. Add rotation
        mesh_transform_ws.rotate(mesh_rotation_ws);
        // 3. Shift up to center sphere to (0, 0, 0)
        mesh_transform_ws.translate(Eigen::Vector3d(radius, 0, 0));
    }

    // Compute LODs
    const double camera_distance_to_ground = (player->get_world_position() - get_world_position()).norm() - static_cast<double>(radius);
    const double normalized_distance       = std::max(1.0, camera_distance_to_ground / (cell_width * (cell_count * 4 + 2)));
    const int    display_lod0_level        = std::min(num_lods - 1, static_cast<int>(std::log2(normalized_distance)));
    const int    display_max_lod           = num_lods; // @TODO : limit max LOD when camera is close to ground
    const double display_lod0_cell_width   = cell_width * std::pow(2.0, display_lod0_level);

    root->tick(delta_time, display_max_lod - display_lod0_level, display_lod0_cell_width);

    // Update planet transform
    current_orbit += static_cast<double>(orbit_speed) * delta_time;
    current_rotation += static_cast<double>(rotation_speed) * delta_time;
    set_local_position(Eigen::Vector3d(std::cos(current_orbit), std::sin(current_orbit), 0) * orbit_distance);
    set_local_rotation(Eigen::Quaterniond(Eigen::AngleAxisd(current_rotation, Eigen::Vector3d::UnitZ())));
}


void Planet::render(Camera& camera) {
    STAT_FRAME("Render Planet");
    SceneComponent::render(camera);

    if (landscape_material->bind()) {
        landscape_material->set_transform("mesh_transform_ws", mesh_transform_ws);
        landscape_material->set_float("radius", radius);
        landscape_material->set_int("cell_count", cell_count);
        landscape_material->set_vec4("debug_vector", debug_vector);
        landscape_material->set_rotation("mesh_rotation_ps", mesh_rotation_ps);
        landscape_material->set_rotation("scene_rotation", get_world_rotation());
        landscape_material->set_texture("grass_color", grass_albedo);
        landscape_material->set_texture("rock_color", rock_albedo);
        landscape_material->set_texture("sand_color", sand_albedo);
        landscape_material->set_texture("grass_normal", grass_normal);
        landscape_material->set_texture("rock_normal", rock_normal);
        landscape_material->set_texture("sand_normal", sand_normal);
        landscape_material->set_texture("grass_mrao", grass_mrao);
        landscape_material->set_texture("rock_mrao", rock_mrao);
        landscape_material->set_texture("sand_mrao", sand_mrao);
        landscape_material->set_texture("water_normal", water_normal);
        landscape_material->set_texture("water_displacement", water_displacement);
        root->render(camera);
    }

    if (display_normals && debug_normal_display_material->bind()) {
        debug_normal_display_material->set_transform("mesh_transform_ws", mesh_transform_ws);
        debug_normal_display_material->set_float("radius", radius);
        debug_normal_display_material->set_int("cell_count", cell_count);
        debug_normal_display_material->set_vec4("debug_vector", debug_vector);
        debug_normal_display_material->set_rotation("mesh_rotation_ps", mesh_rotation_ps);
        debug_normal_display_material->set_rotation("scene_rotation", get_world_rotation());
        root->render(camera);
    }
}
