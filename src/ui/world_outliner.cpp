#include "world_outliner.h"

#include <imgui.h>

#include "world/world.h"

static size_t node_index = 0;

bool WorldOutliner::draw_node(const std::shared_ptr<SceneComponent>& node) {
	bool ok = true;
	int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

	if (node->get_children().empty())
		flags |= ImGuiTreeNodeFlags_Leaf;

	if (selected_node == node)
		flags |= ImGuiTreeNodeFlags_Selected;

	const bool expand = ImGui::TreeNodeEx((node->name + "##" + std::to_string(node_index++)).c_str(), flags);
	if (ImGui::BeginDragDropSource())
	{
		ImGui::Text("%s", node->name.c_str());
		ImGui::SetDragDropPayload("scene_component", &*selected_node, sizeof(SceneComponent*), ImGuiCond_Once);
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (ImGui::AcceptDragDropPayload("scene_component"))
		{
			if (selected_node)
				if (selected_node->get_parent() != &*node)
					node->add_child(selected_node);
				else
				{
					if (selected_node->get_parent() && selected_node->get_parent()->get_parent())
						selected_node->get_parent()->get_parent()->add_child(selected_node);
				}
			ok = false;
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::IsItemClicked())
	{
		selected_node = node;
	}
	if (expand)
	{
		if (ok)
			for (int64_t i = node->get_children().size() - 1; i >= 0; --i)
				if (!draw_node(node->get_children()[i]))
					break;
		ImGui::TreePop();
	}
	return ok;
}


void WorldOutliner::draw()
{
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(104 / 255.f, 57 / 255.f, 2 / 255.f, 1));
	node_index = 0;
	ImGui::Separator();
	for (int64_t i = world->get_scene_root().get_children().size() - 1; i >= 0; --i)
		if (!draw_node(world->get_scene_root().get_children()[i]))
			break;
	ImGui::Separator();
	if
	(selected_node)
	{
		if (ImGui::BeginChild("edit"))
		{
			ImGui::Text("[%s]", selected_node->name.c_str());
			selected_node->draw_ui();
		}
		ImGui::EndChild();
	}
	ImGui::PopStyleColor();
}
