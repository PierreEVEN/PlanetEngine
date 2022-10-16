#pragma once
#include "scene_component.h"

class TextureCube;

class CubemapComponent final : public SceneComponent
{
public:
	CubemapComponent(const std::string& name) : SceneComponent(name)
	{
	}

	void set_texture(const std::shared_ptr<TextureCube>& in_cubemap) { cubemap = in_cubemap; }

	void render(Camera& camera) override;
private:
	std::shared_ptr<TextureCube> cubemap;
};
