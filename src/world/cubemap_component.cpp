

#include "cubemap_component.h"

#include "graphics/material.h"

std::shared_ptr<Material> cubemap_material = nullptr;

void CubemapComponent::render(Camera& camera)
{
	SceneComponent::render(camera);

	if (!cubemap_material) {
		cubemap_material = Material::create("cubemap_material");
		cubemap_material->load_from_source("resources/shaders/cubemap_shader.vs", "resources/shaders/cubemap_shader.fs");
	}

	cubemap_material->bind();
}
