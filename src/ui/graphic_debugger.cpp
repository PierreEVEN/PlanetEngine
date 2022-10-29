#include "graphic_debugger.h"

#include "graphics/framegraph.h"
#include "graphics/render_pass.h"
#include "graphics/texture_image.h"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <ImCurveEdit.h>

#include "world/world.h"
#include "utils/game_settings.h"

GraphicDebugger::GraphicDebugger(const std::shared_ptr<FrameGraph>& in_framegraph)
    : framegraph(in_framegraph) {
    window_name = "graphic debugger";

    expand_node(in_framegraph->get_root());
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

    if (ImGui::BeginChild("frame graph")) {
        for (auto& node : nodes) {
            node.second.draw_node();
        }
    }
    ImGui::EndChild();

}

void GraphicDebugger::expand_node(const std::shared_ptr<RenderPass>& rp) {

    if (nodes.contains(rp))
        return;

    for (const auto& dep : rp->get_dependencies()) {
        expand_node(dep);
    }

    std::vector<Node*> parents;
    for (const auto& parent : rp->get_dependencies()) {
        parents.emplace_back(&nodes[parent]);
    }
    current_pos.y += 220;
    const auto& node = &(nodes[rp] = Node{
                             .position = current_pos, .size = ImVec2(300, 200),
                             .render_pass = rp,
                             .parents = parents
                         });

}

void GraphicDebugger::Node::draw_node() {

    ImGui::BeginGroup();

    const auto& draw_list = ImGui::GetWindowDrawList();
    const auto& min       = ImGui::GetWindowPos();

    draw_list->AddImage(
        reinterpret_cast<ImTextureID>(static_cast<size_t>(render_pass->get_color_attachments()[0]->get_render_target()->id())),
                        ImVec2(position.x + min.x, position.y + min.y), ImVec2(position.x + size.x + min.x, position.y + size.y + min.y),
        ImVec2(0, 1),
        ImVec2(1, 0)
        );

    ImGui::EndGroup();
}
