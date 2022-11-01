#include <imgui.h>
#include <ImGuizmo.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <ui/ui.h>

#include "asset_manager_ui.h"
#include "camera.h"
#include "graphic_debugger.h"
#include "info_ui.h"
#include "session_frontend.h"
#include "world_outliner.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/framegraph.h"
#include "utils/game_settings.h"
#include "utils/profiler.h"
#include "world/world.h"

static bool show_demo_window = false;

class ImGuiDemoWindow : public ImGuiWindow {
public:
    ImGuiDemoWindow() {
        window_name = "demo window";
    }

    void draw() override {
        ImGui::ShowDemoWindow(&open);

        if (!open)
            close();
    }

    bool open = true;
};

namespace ui {
void draw() {
    STAT_FRAME("ImGui_UI");
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                exit(EXIT_SUCCESS);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("World outliner"))
                ImGuiWindow::create_window<WorldOutliner>(
                    &Engine::get().get_world());
            ImGui::Separator();
            if (ImGui::MenuItem("Texture view"))
                ImGuiWindow::create_window<TextureManagerUi>();
            if (ImGui::MenuItem("Material view"))
                ImGuiWindow::create_window<MaterialManagerUi>();
            if (ImGui::MenuItem("Mesh view"))
                ImGuiWindow::create_window<MeshManagerUi>();
            ImGui::Separator();
            if (ImGui::MenuItem("Session frontend"))
                ImGuiWindow::create_window<SessionFrontend>();
            if (!FrameGraph::registry().empty())
                if (ImGui::MenuItem("Graphic debugger"))
                    ImGuiWindow::create_window<GraphicDebugger>(FrameGraph::registry()[0]);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("System information"))
                ImGuiWindow::create_window<InfoUi>();
            if (ImGui::MenuItem("Demo window"))
                ImGuiWindow::create_window<ImGuiDemoWindow>();
            ImGui::EndMenu();
        }

        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 100, 0));
        ImGui::Text("%f fps", 1.0 / Engine::get().get_world().get_delta_seconds());

        ImGui::EndMainMenuBar();
    }

    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    ImGuiWindow::draw_all();
}
}

static std::vector<std::shared_ptr<ImGuiWindow>> imgui_windows;
static size_t                                    current_window_id = 0;

size_t ImGuiWindow::make_window_id() {
    return current_window_id++;
}

void ImGuiWindow::register_window_internal(std::shared_ptr<ImGuiWindow> window) {
    imgui_windows.emplace_back(window);
}

void ImGuiWindow::draw_all() {
    for (int64_t i = static_cast<int64_t>(imgui_windows.size()) - 1; i >= 0; --i)
        if (!imgui_windows[i]->is_open)
            imgui_windows.erase(imgui_windows.begin() + i);

    for (size_t i = 0; i < imgui_windows.size(); ++i)
        imgui_windows[i]->draw_internal();
}

void ImGuiWindow::draw_internal() {
    if (GameSettings::get().fullscreen)
        return;

    if (ImGui::Begin((window_name + "##" + std::to_string(window_id)).c_str(), &is_open))
        draw();
    ImGui::End();
}
