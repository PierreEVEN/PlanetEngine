#include "scene_component.h"

#include <Eigen/Core>

#include "ui/widgets.h"

Eigen::Quaterniond SceneComponent::get_world_rotation()
{
	if (parent)
		return Eigen::Quaterniond((parent->get_world_transform() * local_rotation).rotation());

	return local_rotation;
}

void SceneComponent::add_child(const std::shared_ptr<SceneComponent>& new_child)
{
	if (!new_child)
		return;

	if (new_child->parent == this)
		return;
	
	new_child->detach();
	new_child->parent = this;
	children.emplace_back(new_child);
}

void SceneComponent::detach()
{
	if (parent)
		for (size_t i = 0; i < parent->children.size(); ++i)
			if (parent->children[i].get() == this)
				parent->children.erase(parent->children.begin() + i);
	parent = nullptr;
}

void SceneComponent::draw_ui()
{
	Eigen::Vector3d location = get_local_position();
	if (ui::position_edit(location, "position"))
		set_local_position(location);

	Eigen::Quaterniond rotation = get_local_rotation();
	if (ui::rotation_edit(rotation, "rotation"))
		set_local_rotation(rotation);

	Eigen::Vector3d scale = get_local_scale();
	if (ui::position_edit(scale, "scale"))
		set_local_scale(scale);
}

void SceneComponent::tick_internal(double delta_time)
{
	tick(delta_time);

	for (const auto& child : children)
		child->tick_internal(delta_time);
}

void SceneComponent::render_internal(Camera& camera)
{
	render(camera);

	for (const auto& child : children)
		child->render_internal(camera);
}
