#include "viewport.h"

#include <imgui.h>
#include <iostream>
#include <texture2d.h>

#include "engine/engine.h"
#include "engine/renderer.h"

Viewport::Viewport()
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
		reinterpret_cast<ImTextureID>(static_cast<size_t>(Engine::get().get_renderer().get_resolve_texture().id())),
		ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
}

void Viewport::on_fullscreen(bool fullscreen)
{
	if (!fullscreen)
		Engine::get().get_renderer().resize_framebuffer_internal(nullptr, last_viewport_res.x(), last_viewport_res.y());
}
