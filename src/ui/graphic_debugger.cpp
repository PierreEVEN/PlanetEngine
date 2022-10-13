#include "graphic_debugger.h"

#include <fbo.h>
#include <imgui.h>
#include <GLFW/glfw3.h>

#include "world.h"
#include "engine/engine.h"
#include "engine/renderer.h"

static bool enable_vsync = true;

GraphicDebugger::GraphicDebugger()
{
	window_name = "graphic debugger";
}

void GraphicDebugger::draw()
{
	if (ImGui::Checkbox("Enable VSync", &enable_vsync))
		glfwSwapInterval(enable_vsync ? 1 : 0);

	if (ImGui::Checkbox("Wireframe", &Engine::get().get_renderer().wireframe))
		glfwSwapInterval(enable_vsync ? 1 : 0);

	ImGui::DragInt("Framerate limit", &Engine::get().get_world().framerate_limit);
	if (Engine::get().get_world().framerate_limit < 0) Engine::get().get_world().framerate_limit = 0;

	const auto framebuffer = Engine::get().get_renderer().framebuffer();
	const float available_width = ImGui::GetContentRegionAvail().x;
	for (int i = 0; i < framebuffer->nb_textures(); ++i)
	{
		const float ratio = available_width / framebuffer->width();
		ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(framebuffer->texture(i)->id())),
		             ImVec2(framebuffer->width() * ratio, framebuffer->height() * ratio), ImVec2(0, 1),
		             ImVec2(1, 0));
	}
	if (framebuffer->depth_texture())
	{
		const float ratio = available_width / framebuffer->width();
		ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(framebuffer->depth_texture()->id())),
		             ImVec2(framebuffer->width() * ratio, framebuffer->height() * ratio), ImVec2(0, 1),
		             ImVec2(1, 0));
	}
}
