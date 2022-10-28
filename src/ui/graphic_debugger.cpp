#include "graphic_debugger.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include "world/world.h"
#include "utils/game_settings.h"

GraphicDebugger::GraphicDebugger(const std::shared_ptr<FrameGraph>& in_framegraph)
    : framegraph(in_framegraph) {
    window_name = "graphic debugger";
}

void GraphicDebugger::draw() {
    if (ImGui::Checkbox("Enable VSync", &GameSettings::get().v_sync))
        glfwSwapInterval(GameSettings::get().v_sync ? 1 : 0);

    ImGui::Checkbox("Wireframe", &GameSettings::get().wireframe);

    ImGui::SliderFloat("Bloom intensity", &GameSettings::get().bloom_intensity, 0, 3);
    ImGui::SliderFloat("Exposure", &GameSettings::get().exposure, 0.1f, 4);
    ImGui::SliderFloat("Gamma", &GameSettings::get().gamma, 0.5f, 4);

    ImGui::DragInt("Framerate limit", &GameSettings::get().max_fps);
    if (GameSettings::get().max_fps < 0)
        GameSettings::get().max_fps = 0;
}
