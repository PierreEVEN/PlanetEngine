#include "scene_component.h"

#include <Eigen/Core>

Eigen::Quaterniond SceneComponent::get_world_rotation()
{
	if (parent)
		return Eigen::Quaterniond((parent->get_world_transform() * local_rotation).rotation());

	return local_rotation;
}

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
