#include "texture_viewer.h"

#include <imgui.h>

#include "graphics/texture_image.h"

TextureViewer::TextureViewer(TextureBase* in_texture) : texture(in_texture)
{
	window_name = "texture viewer";
}

void TextureViewer::draw()
{
	if (texture)
	{

		ImGui::LabelText(texture->name.c_str(), "name");
		ImGui::LabelText(std::to_string(texture->width()).c_str(), "width");
		ImGui::LabelText(std::to_string(texture->height()).c_str(), "height");
		ImGui::LabelText(std::to_string(texture->id()).c_str(), "opengl id");
		const float width = ImGui::GetContentRegionAvail().x;
		ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(texture->id())),
		             ImVec2(width, texture->height() / static_cast<float>(texture->width()) * width),
		             ImVec2(0, 1), ImVec2(1, 0));
	}
}
