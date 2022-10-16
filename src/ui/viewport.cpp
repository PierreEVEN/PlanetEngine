#include "viewport.h"

#include <imgui.h>
#include <texture_interface.h>

#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/texture_image.h"

Viewport::Viewport(const std::shared_ptr<EZCOGL::TextureInterface>& texture): framebuffer_texture(texture)
{
	window_name = "Viewport";
	Engine::get().get_renderer().on_fullscreen.add_object(this, &Viewport::on_fullscreen);
}

Viewport::~Viewport()
{
	Engine::get().get_renderer().on_fullscreen.clear_object(this);
}

void Viewport::draw()
{
	const auto& new_res = ImGui::GetContentRegionAvail();
	if (last_viewport_res != Eigen::Vector2i{new_res.x, new_res.y} && !Engine::get().get_renderer().is_fullscreen())
	{
		last_viewport_res = Eigen::Vector2i{new_res.x, new_res.y};
		Engine::get().get_renderer().resize_framebuffer_internal(nullptr, static_cast<int>(new_res.x), static_cast<int>(new_res.y));
	}
	ImGui::Image(
		reinterpret_cast<ImTextureID>(static_cast<size_t>(framebuffer_texture->id_interface())),
		ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
}

void Viewport::on_fullscreen(bool fullscreen)
{
	if (!fullscreen)
		Engine::get().get_renderer().resize_framebuffer_internal(nullptr, last_viewport_res.x(), last_viewport_res.y());
}
