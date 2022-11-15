#include "planet_ocean.h"

#include "planet.h"
#include "graphics/camera.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/primitives.h"
#include "graphics/render_pass.h"
#include "utils/game_settings.h"

static std::shared_ptr<Mesh> ocean_mesh = nullptr;

PlanetOcean::PlanetOcean(const std::shared_ptr<Planet>& owning_planet)
    : SceneComponent("Ocean_" + owning_planet->name) {
    
    draw_group = DrawGroup::from<DrawGroup_Translucency>();

    ocean_material = Material::create("Ocean Shader", "resources/shaders/water_shader.vs", "resources/shaders/water_shader.fs");
    if (!ocean_mesh) {
        ocean_mesh = primitives::grid_plane(1024, 1024);
    }
    grid_mesh = ocean_mesh;
}

void PlanetOcean::render(Camera& camera, const DrawGroup& draw_group, const std::shared_ptr<RenderPass>& render_pass) {
    SceneComponent::render(camera, draw_group, render_pass);

    if (GameSettings::get().wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    ocean_material->bind();
    render_pass->bind_dependencies_to_material(ocean_material);
    ocean_material->set_float("z_near", static_cast<float>(camera.z_near()));
    grid_mesh->draw();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
