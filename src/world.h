#pragma once
#include <memory>

#include "world/scene_component.h"

class Camera;

class World final
{
public:
	World();
	~World();

	void tick_world();

	void render_world() const;

	[[nodiscard]] const std::shared_ptr<Camera>& get_camera() const { return camera; }

	[[nodiscard]] SceneComponent& get_scene_root() {
		return *root_component;
	}

private:
	uint32_t world_uniform;
	std::shared_ptr<Camera> camera;
	std::unique_ptr<SceneComponent> root_component;
	double last_time;
};
