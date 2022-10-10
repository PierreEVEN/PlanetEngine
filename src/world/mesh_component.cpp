#include "mesh_component.h"

#include <GL/gl3w.h>


#include "camera.h"
#include "engine/engine.h"
#include "engine/renderer.h"
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
