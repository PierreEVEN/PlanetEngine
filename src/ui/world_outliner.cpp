#include "world_outliner.h"

#include <imgui.h>

#include "world.h"

static size_t node_index = 0;

static std::shared_ptr<SceneComponent> selected_node = nullptr;

static void draw_node(const std::shared_ptr<SceneComponent>& parent)
{
	int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

	if (parent->get_children().empty())
		flags |= ImGuiTreeNodeFlags_Leaf;


	const bool expand = ImGui::TreeNodeEx((parent->name + "##" + std::to_string(node_index++)).c_str(), flags);
	if (ImGui::BeginDragDropSource())
	{
		ImGui::Text("%s", parent->name.c_str());
		ImGui::SetDragDropPayload("scene_component", &parent, sizeof(parent), ImGuiCond_Once);
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (ImGui::AcceptDragDropPayload("scene_component"))
			if (selected_node)
				if (selected_node->get_parent() != &*parent)
					parent->add_child(selected_node);
				else
					if(parent->get_parent())
					parent->get_parent()->add_child(selected_node);
		ImGui::EndDragDropTarget();
	}


	if (ImGui::IsItemClicked())
	{
		selected_node = parent;
	}
	if (expand)
	{
		for (const auto& child : parent->get_children())
		{
			draw_node(child);
		}
		ImGui::TreePop();
	}
}


void WorldOutliner::draw()
{
	node_index = 0;
	ImGui::Separator();
	for (const auto& item : world->get_scene_root().get_children())
		draw_node(item);
	ImGui::Separator();
	if
	(selected_node)
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg,
		                      ImGui::ColorConvertFloat4ToU32(ImVec4(63 / 256.f, 72 / 256.f, 69 / 256.f, 1)));
		if (ImGui::BeginChild("edit"))
		{
			ImGui::Text("[%s]", selected_node->name.c_str());
			selected_node->draw_ui();
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}
}
