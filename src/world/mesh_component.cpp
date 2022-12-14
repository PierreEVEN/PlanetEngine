#include "mesh_component.h"

#include <imgui.h>
#include <GL/gl3w.h>


#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/camera.h"
#include "graphics/mesh.h"
#include "graphics/material.h"
#include "utils/game_settings.h"

void MeshComponent::render(Camera& camera, const DrawGroup& draw_group, const std::shared_ptr<RenderPass>& render_pass) {
    SceneComponent::render(camera, draw_group, render_pass);

    if (!mesh || !material)
        return;

    if (GameSettings::get().wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    material->bind();
    auto transform = get_world_transform();
    transform.translate(-camera.get_world_position());
    material->set_model_transform(transform);
    mesh->draw();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void MeshComponent::draw_ui() {
    SceneComponent::draw_ui();
    ImGui::Text("mesh : %s", mesh ? mesh->name.c_str() : "none");
    ImGui::Text("material : %s", material ? material->name.c_str() : "none");
}
