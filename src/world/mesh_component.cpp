#include "mesh_component.h"

#include "camera.h"
#include "graphics/mesh.h"
#include "graphics/material.h"

void MeshComponent::render(Camera& camera)
{
	SceneComponent::render(camera);

	if (!mesh || !material)
		return;

	material->use();
	auto transform = get_world_transform();
	transform.translate(-camera.get_world_position());
	material->set_model_transform(transform);
	mesh->draw();
}
