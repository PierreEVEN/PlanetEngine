#include <fbo.h>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <ui/ui.h>

#include "world.h"

static bool show_debug_window = false;
static bool enable_vsync = true;
static bool show_demo_window = false;

namespace ui
{
	void draw(const Renderer& renderer, const World& world, const std::shared_ptr<EZCOGL::FBO_DepthTexture>& g_buffer)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					exit(0);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Debug"))
			{
				ImGui::Checkbox("framebuffers", &show_debug_window);
				if (ImGui::Checkbox("Enable VSync", &enable_vsync))
					glfwSwapInterval(enable_vsync ? 1 : 0);
				ImGui::Checkbox("demo window", &show_demo_window);
				ImGui::EndMenu();
			}

			ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 100, 0));
			ImGui::Text("%f fps", 1.0f / world.get_delta_seconds());

			ImGui::EndMainMenuBar();
		}

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		if (show_debug_window)
		{
			const float available_width = ImGui::GetContentRegionAvail().x;
			if (ImGui::Begin("framebuffers", &show_debug_window))
			{
				for (int i = 0; i < g_buffer->nb_textures(); ++i)
				{
					const float ratio = available_width / g_buffer->width();
					ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(g_buffer->texture(i)->id())),
					             ImVec2(g_buffer->width() * ratio, g_buffer->height() * ratio), ImVec2(0, 1),
					             ImVec2(1, 0));
				}
				if (g_buffer->depth_texture())
				{
					const float ratio = available_width / g_buffer->width();
					ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<size_t>(g_buffer->depth_texture()->id())),
					             ImVec2(g_buffer->width() * ratio, g_buffer->height() * ratio), ImVec2(0, 1),
					             ImVec2(1, 0));
				}
			}
			ImGui::End();
		}
	}
}
