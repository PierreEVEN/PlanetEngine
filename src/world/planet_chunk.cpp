#include "planet_chunk.h"

#include "planet.h"
#include "graphics/camera.h"
#include "graphics/compute_shader.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/storage_buffer.h"
#include "graphics/texture_image.h"
#include "utils/game_settings.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"

PlanetChunk::PlanetChunk(Planet& in_parent, uint32_t in_lod_level, uint32_t in_my_level)
    : num_lods(in_lod_level), current_lod(in_my_level), planet(in_parent) {
}

void PlanetChunk::regenerate(int32_t in_cell_number) {
    cell_number = in_cell_number;
    {
        const int map_size = cell_number * 4 + 5;
        if (!chunk_height_map || chunk_height_map->width() != map_size) {
            GL_CHECK_ERROR();
            chunk_height_map =
                Texture2D::create("heightmap_LOD_" + std::to_string(current_lod),
                                  {.wrapping = TextureWrapping::ClampToEdge, .filtering_mag = TextureMagFilter::Nearest, .filtering_min = TextureMinFilter::Nearest});
            chunk_height_map->set_data(map_size, map_size, ImageFormat::RG_F32);
            GL_CHECK_ERROR();
        }
        if (!chunk_normal_map || chunk_normal_map->width() != map_size) {
            GL_CHECK_ERROR();
            chunk_normal_map =
                Texture2D::create("normal_LOD_" + std::to_string(current_lod),
                                  {.wrapping = TextureWrapping::ClampToEdge, .filtering_mag = TextureMagFilter::Nearest, .filtering_min = TextureMinFilter::Nearest});
            chunk_normal_map->set_data(map_size, map_size, ImageFormat::RG_F16);
            GL_CHECK_ERROR();
        }

        rebuild_maps();
    }
    if (child)
        child->regenerate(cell_number);
}

void PlanetChunk::tick(double delta_time, int in_num_lods, double in_width) {
    cell_size = in_width; // in_width;
    // Create or destroy children
    num_lods = in_num_lods;
    if (!child && current_lod + 1 < num_lods) {
        child = std::make_shared<PlanetChunk>(planet, num_lods, current_lod + 1);
        child->regenerate(cell_number);
    }
    if (child && current_lod >= num_lods - 1)
        child = nullptr;

    if (child)
        child->tick(delta_time, num_lods, cell_size * 2);

    STAT_FRAME("Planet Tick LOD :" + std::to_string(current_lod));
    // Compute camera position in local space
    const Eigen::Vector3d camera_local_position = planet.inv_mesh_rotation_ws * (planet.player->get_world_position() - planet.get_world_position());

    const Eigen::Vector3d temp = Eigen::Vector3d(0,
                                                 Eigen::Vector3d(camera_local_position.x(), camera_local_position.y(), 0).normalized().y(),
                                                 Eigen::Vector3d(camera_local_position.x(), camera_local_position.y(), camera_local_position.z()).normalized().z());

    // Convert linear position to position on sphere
    Eigen::Vector3d local_location = Eigen::Vector3d(0, asin(std::clamp(temp.y(), -1.0, 1.0)), asin(std::clamp(temp.z(), -1.0, 1.0))) * planet.radius;

    const double snapping = cell_size * 2;
    chunk_position        = Eigen::Vector3d(std::round(local_location.y() / snapping + 0.5) - 0.5, 0, std::round(local_location.z() / snapping + 0.5) - 0.5) * snapping;

    mesh_transform_cs = Eigen::Affine3d::Identity();
    mesh_transform_cs.translate(chunk_position);
    mesh_transform_cs.scale(cell_size);
    if (current_lod != 0) {
        if (local_location.y() >= chunk_position.x() && local_location.z() < chunk_position.z())
            mesh_transform_cs.rotate(Eigen::AngleAxisd(-M_PI / 2, Eigen::Vector3d::UnitY()));
        else if (local_location.y() < chunk_position.x() && local_location.z() >= chunk_position.z())
            mesh_transform_cs.rotate(Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitY()));
        else if (local_location.y() >= chunk_position.x() && local_location.z() >= chunk_position.z())
            mesh_transform_cs.rotate(Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitY()));
    }

    rebuild_maps();
}

void PlanetChunk::render(Camera& camera) {
    if (child)
        child->render(camera);
    GL_CHECK_ERROR();
    STAT_FRAME("Render planet lod " + std::to_string(current_lod));

    // Set uniforms
    planet.landscape_material->set_transform("mesh_transform_cs", mesh_transform_cs);
    planet.landscape_material->set_texture("height_map", chunk_height_map);
    planet.landscape_material->set_texture("normal_map", chunk_normal_map);

    // Draw
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GameSettings::get().wireframe ? GL_LINE : GL_FILL);
    if (current_lod == 0)
        planet.root_mesh->draw();
    else
        planet.child_mesh->draw();
    if (planet.double_sided) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glFrontFace(GL_CW);
        if (current_lod == 0)
            planet.root_mesh->draw();
        else
            planet.child_mesh->draw();
        glFrontFace(GL_CCW);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void PlanetChunk::rebuild_maps() {
    if (planet.freeze_updates && !force_rebuild)
        return;

    GL_CHECK_ERROR();
    STAT_FRAME("rebuild landscape map");

    auto test = Eigen::Affine3d::Identity();
    test.rotate(planet.mesh_rotation_ps);

    const LandscapeChunkData chunk_data{.Chunk_LocalTransform = mesh_transform_cs.cast<float>().matrix(),
                                        .Chunk_PlanetModel = (planet.get_world_transform().inverse() * planet.mesh_transform_ws).cast<float>().matrix(),
                                        .Chunk_LocalOrientation = test.cast<float>().matrix(),
                                        .Chunk_PlanetRadius = planet.radius,
                                        .Chunk_CellWidth = static_cast<float>(cell_size),
                                        .Chunk_CellCount = cell_number,
                                        .Chunk_CurrentLOD = static_cast<int32_t>(current_lod)};

    if (chunk_data == last_chunk_data && !force_rebuild)
        return;

    force_rebuild = false;

    GL_CHECK_ERROR();
    const auto ssbo = StorageBuffer::create("PlanetChunkData");
    ssbo->set_data(chunk_data);
    last_chunk_data = chunk_data;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo->id());

    // Compute heightmaps
    planet.compute_positions->bind();
    planet.compute_positions->bind_texture(chunk_height_map, BindingMode::Out, 0);
    planet.compute_positions->execute(chunk_height_map->width(), chunk_height_map->height(), 1);

    // Fix seams
    planet.compute_fix_seams->bind();
    planet.compute_fix_seams->bind_texture(chunk_height_map, BindingMode::InOut, 0);
    planet.compute_fix_seams->execute(chunk_height_map->width(), chunk_height_map->height(), 1);

    // Compute normals
    planet.compute_normals->bind();
    planet.compute_normals->bind_texture(chunk_height_map, BindingMode::In, 0);
    planet.compute_normals->bind_texture(chunk_normal_map, BindingMode::Out, 1);
    planet.compute_normals->execute(chunk_height_map->width(), chunk_height_map->height(), 1);

    GL_CHECK_ERROR();
}

void PlanetChunk::force_rebuild_maps() {
    force_rebuild = true;
    if (child)
        child->force_rebuild_maps();
}

bool PlanetChunk::LandscapeChunkData::operator==(const LandscapeChunkData& other) const {
    return Chunk_LocalTransform == other.Chunk_LocalTransform && Chunk_LocalOrientation == other.Chunk_LocalOrientation && Chunk_PlanetRadius == other.Chunk_PlanetRadius &&
           Chunk_CellWidth == other.Chunk_CellWidth && Chunk_CellCount == other.Chunk_CellCount && Chunk_CurrentLOD == other.Chunk_CurrentLOD;
}
