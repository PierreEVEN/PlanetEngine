#include "mesh_component.h"

#include <imgui.h>
#include <GL/gl3w.h>


#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/camera.h"
#include "graphics/mesh.h"
#include "graphics/material.h"

void MeshComponent::render(Camera& camera)
{
	SceneComponent::render(camera);

	if (!mesh || !material)
		return;

	if (Engine::get().get_renderer().wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	material->bind();
	auto transform = get_world_transform();
	transform.translate(-camera.get_world_position());
	material->set_model_transform(transform);
	mesh->draw();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void MeshComponent::draw_ui()
{
	SceneComponent::draw_ui();
	ImGui::Text("mesh : %s", mesh ? mesh->name.c_str() : "none");
	ImGui::Text("material : %s", material ? material->name.c_str() : "none");
}
