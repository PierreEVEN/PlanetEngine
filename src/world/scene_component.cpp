#include "scene_component.h"

void SceneComponent::tick_internal(double delta_time)
{
	tick(delta_time);

	for (const auto& child : children)
		child->tick_internal(delta_time);
}

void SceneComponent::render_internal()
{
	render();

	for (const auto& child : children)
		child->render_internal();
}
