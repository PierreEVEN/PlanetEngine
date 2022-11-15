#pragma once
#include <memory>

#include "world/scene_component.h"

class RenderPass;

class World final
{
public:
	World();

	void tick_world();

	void render_world(const DrawGroup& draw_group, const std::shared_ptr<Camera>& camera, const std::shared_ptr<RenderPass>& render_pass) const;
	
	[[nodiscard]] SceneComponent& get_scene_root() const
	{
		return *root_component;
	}

	[[nodiscard]] double get_delta_seconds() const
	{
		return delta_seconds;
	}
	
private:
	std::unique_ptr<SceneComponent> root_component;
	double last_time = -1;
	double delta_seconds = -1;
};
