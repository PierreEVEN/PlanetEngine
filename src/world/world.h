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

	[[nodiscard]] std::shared_ptr<Camera> get_camera() const;

	[[nodiscard]] SceneComponent& get_scene_root() const
	{
		return *root_component;
	}

	[[nodiscard]] double get_delta_seconds() const
	{
		return delta_seconds;
	}

	int framerate_limit = 0;

private:
	uint32_t world_uniform;
	std::shared_ptr<Camera> camera;
	std::unique_ptr<SceneComponent> root_component;
	double last_time;
	double delta_seconds;
};
