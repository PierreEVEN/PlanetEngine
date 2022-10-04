

#include "world_outliner.h"

#include <imgui.h>

#include "world.h"

static size_t node_index = 0;

static void draw_node(SceneComponent* parent)
{
	if (ImGui::TreeNode((parent->name + "##" + std::to_string(node_index)).c_str()))
	{
		for (const auto& child : parent->get_children())
		{
			draw_node(child.get());
		}
		ImGui::TreePop();
	}
}


void WorldOutliner::draw()
{
	ImGui::Separator();
	draw_node(&world->get_scene_root());
}
