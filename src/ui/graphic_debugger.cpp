#include "graphic_debugger.h"

#include <fbo.h>
#include <imgui.h>
#include <GLFW/glfw3.h>

#include "world/world.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "utils/game_settings.h"

GraphicDebugger::GraphicDebugger()
{
	window_name = "graphic debugger";
}

void GraphicDebugger::draw()
{
	if (ImGui::Checkbox("Enable VSync", &GameSettings::get().v_sync))
		glfwSwapInterval(GameSettings::get().v_sync ? 1 : 0);

	ImGui::Checkbox("Wireframe", &GameSettings::get().wireframe);

	ImGui::SliderFloat("Bloom intensity", &GameSettings::get().bloom_intensity, 0, 3);
	ImGui::SliderFloat("Exposure", &GameSettings::get().exposure, 0.1f, 4);
	ImGui::SliderFloat("Gamma", &GameSettings::get().gamma, 0.5f, 4);

	ImGui::DragInt("Framerate limit", &GameSettings::get().max_fps);
	if (GameSettings::get().max_fps < 0) GameSettings::get().max_fps = 0;

	const auto framebuffer = Engine::get().get_renderer().framebuffer();
	const float available_width = ImGui::GetContentRegionAvail().x;
	for (int i = 0; i < framebuffer->nb_textures(); ++i)
	{
		const float ratio = available_width / framebuffer->width();
		ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(framebuffer->texture(i)->id_interface())),
		             ImVec2(framebuffer->width() * ratio, framebuffer->height() * ratio), ImVec2(0, 1),
		             ImVec2(1, 0));
	}
	if (framebuffer->depth_texture())
	{
		const float ratio = available_width / framebuffer->width();
		ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(framebuffer->depth_texture()->id_interface())),
		             ImVec2(framebuffer->width() * ratio, framebuffer->height() * ratio), ImVec2(0, 1),
		             ImVec2(1, 0));
	}
}
