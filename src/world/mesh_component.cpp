#include "mesh_component.h"

#include "../graphics/mesh.h"
#include "../graphics/material.h"

void MeshComponent::render()
{
	SceneComponent::render();

	if (!mesh || !material)
		return;

	material->use();
	material->set_model_transform(get_world_transform());
	mesh->draw();
}
