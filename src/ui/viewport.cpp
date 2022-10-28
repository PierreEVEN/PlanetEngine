#include "viewport.h"

#include <imgui.h>
#include <texture_interface.h>

#include "graphics/texture_image.h"

Viewport::Viewport(const std::shared_ptr<Texture2D>& texture)
    : framebuffer_texture(texture) {
    window_name = "Viewport";
}

void Viewport::draw() {
    const auto& new_res = ImGui::GetContentRegionAvail();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        viewport_width  = static_cast<uint32_t>(new_res.x);
        viewport_height = static_cast<uint32_t>(new_res.y);
    }
    ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(framebuffer_texture->id())), ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
}