#include "asset_manager_ui.h"

#include <filesystem>

#include "engine/asset_manager.h"
#include "engine/engine.h"
#include <imgui.h>
#include <iostream>

#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/texture_image.h"

int parse_error_message(const std::string& message)
{
	bool record = false;
	std::string l_str;
	for (const auto& c : message)
	{
		if (c == '(')
		{
			record = true;
			continue;
		}
		if (c == ')')
		{
			return std::atoi(l_str.c_str());
		}

		if (record)
		{
			l_str += c;
		}
	}
	return 0;
}

void open_in_ide(const std::string& file_path, int line)
{
	std::string text = "code --goto \"" + std::filesystem::absolute(file_path).string() + ":" + std::to_string(line) +
		":0\"";

#if _WIN32
	for (auto& c : text)
		if (c == '/')
			c = '\\';
#endif


	std::cout << "execute command : " << text << std::endl;
	system(text.c_str());
}

static void mesh_manager()
{
	const float total_width = ImGui::GetContentRegionAvail().x;

	constexpr float field_a_width = 200;
	constexpr float field_b_width = 300;
	constexpr float field_c_width = 450;
	constexpr float field_d_width = 500;
	constexpr float field_e_width = 600;

	ImGui::Text("name");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_a_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("vertices");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_b_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("indices");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_c_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("vao id");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_d_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("vbo id");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_e_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("ebo id");

	ImGui::Separator();

	for (const auto& mesh : Engine::get().get_asset_manager().get_meshes())
	{
		ImGui::BeginGroup();
		ImGui::Text("%s", mesh->name.c_str());
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(std::max(field_a_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		ImGui::Text("%d", mesh->vertex_count());
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(std::max(field_b_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		ImGui::Text("%d", mesh->index_count());
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(std::max(field_c_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		ImGui::Text("%d", mesh->vao_id());
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(std::max(field_d_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		ImGui::Text("%d", mesh->vbo_id());
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(std::max(field_e_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		ImGui::Text("%d", mesh->ebo_id());
		ImGui::EndGroup();
	}
}

void display_file_hierarchy(const ShaderSource& source)
{
	if (ImGui::MenuItem("open"))
	{
		open_in_ide(source.get_path(), 0);
	}
	ImGui::Separator();
	for (const auto& file : source.get_dependencies())
	{
		if (file->get_dependencies().empty())
		{
			if (ImGui::MenuItem(file->get_file_name().c_str()))
				open_in_ide(file->get_path(), 0);
		}
		else if (ImGui::BeginMenu(file->get_file_name().c_str()))
		{
			display_file_hierarchy(*file);
			ImGui::EndMenu();
		}
	}
}


static void material_manager()
{
	const float total_width = ImGui::GetContentRegionAvail().x;

	constexpr float field_a_width = 140;
	constexpr float field_b_width = 180;
	constexpr float field_c_width = 330;
	constexpr float field_d_width = 420;
	constexpr float field_e_width = 500;

	ImGui::Text("name");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_a_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("status");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_b_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("hot reload");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_c_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("opengl id");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_d_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("auto-reload");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(std::max(field_e_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
	ImGui::SameLine();
	ImGui::Text("open in vs-code");

	ImGui::Separator();

	for (const auto& material : Engine::get().get_asset_manager().get_materials())
	{
		ImGui::BeginGroup();
		ImGui::Text("%s", material->name.c_str());
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(std::max(field_a_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		if (!material->program_id())
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0.5, 0.5, 1.0));
			if (ImGui::Button("error", ImVec2(80, 0)))
			{
				/*
				const int line = parse_error_message(material->last_error);
				if (material->last_error.length() > 0 && material->last_error[0] == 'F')
					open_in_ide(material->get_fragment_source(), line);
				else
					open_in_ide(material->get_vertex_source(), line);
					*/
			}
			ImGui::PopStyleColor();
		}
		else
			ImGui::Text("%s", "ready");
		ImGui::SameLine();

		ImGui::Dummy(ImVec2(std::max(field_b_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		if (ImGui::Button(("reload##" + std::to_string(material->program_id())).c_str(), ImVec2(120, 0)))
			material->check_updates();
		ImGui::SameLine();

		ImGui::Dummy(ImVec2(std::max(field_c_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		ImGui::Text("%d", material->program_id());
		ImGui::SameLine();

		ImGui::Dummy(ImVec2(std::max(field_d_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		ImGui::Checkbox(("##" + std::to_string(material->program_id())).c_str(), &material->auto_reload);
		ImGui::SameLine();

		ImGui::Dummy(ImVec2(std::max(field_e_width - total_width + ImGui::GetContentRegionAvail().x, 0.f), 0));
		ImGui::SameLine();
		if (ImGui::Button(("vertex##" + std::to_string(material->program_id())).c_str(), ImVec2(80, 0)))
			ImGui::OpenPopup(("dependencies_vertex##" + std::to_string(material->program_id())).c_str());
		if (ImGui::BeginPopup(("dependencies_vertex##" + std::to_string(material->program_id())).c_str()))
		{
			display_file_hierarchy(material->get_vertex_source());
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(("fragment##" + std::to_string(material->program_id())).c_str(), ImVec2(80, 0)))
			ImGui::OpenPopup(("dependencies_fragment##" + std::to_string(material->program_id())).c_str());
		if (ImGui::BeginPopup(("dependencies_fragment##" + std::to_string(material->program_id())).c_str()))
		{
			display_file_hierarchy(material->get_fragment_source());
			ImGui::EndPopup();
		}
		ImGui::EndGroup();
		if (!material->program_id() && ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			//ImGui::Text("%s", material->last_error.c_str());
			ImGui::EndTooltip();
		}
	}
}

static void texture_manager()
{
	const float available_width = ImGui::GetContentRegionAvail().x;
	const int width_items = static_cast<int>(available_width / 70);
	ImGui::Columns(std::max(width_items, 1), "", false);

	for (const auto& texture : Engine::get().get_asset_manager().get_textures())
	{
		const int base_res = std::max(texture->width(), texture->height());
		const int res = std::min(base_res, 800);
		const float ratio = static_cast<float>(res) / static_cast<float>(base_res);

		ImGui::BeginGroup();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.2f, 0.8f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 4));
		ImGui::ImageButton(reinterpret_cast<ImTextureID>(static_cast<size_t>(texture->id())), ImVec2(64, 64),
		                   ImVec2(0, 1), ImVec2(1, 0));
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::TextWrapped(texture->name.c_str());
		ImGui::EndGroup();
		ImGui::NextColumn();

		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::LabelText(texture->name.c_str(), "name");
			ImGui::LabelText(std::to_string(texture->width()).c_str(), "width");
			ImGui::LabelText(std::to_string(texture->height()).c_str(), "height");
			ImGui::LabelText(std::to_string(texture->id()).c_str(), "opengl id");
			ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(texture->id())),
			             ImVec2(texture->width() * ratio, texture->height() * ratio),
			             ImVec2(0, 1), ImVec2(1, 0));
			ImGui::EndTooltip();
		}
	}
	ImGui::Columns(1);
}

void TextureManagerUi::draw()
{
	texture_manager();
}

void MaterialManagerUi::draw()
{
	material_manager();
}

void MeshManagerUi::draw()
{
	mesh_manager();
}
