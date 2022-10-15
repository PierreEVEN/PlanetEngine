#pragma once
#include "scene_component.h"


class CubemapComponent final : public SceneComponent
{
private:
	CubemapComponent(const std::string& name) : SceneComponent(name) {}
};

class TextureCube
{
public:
	static std::shared_ptr<TextureCube> create(const std::string& name)
	{
		return nullptr;
	}
};