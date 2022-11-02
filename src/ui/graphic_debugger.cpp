#include "graphic_debugger.h"

#include "graphics/framegraph.h"
#include "graphics/render_pass.h"
#include "graphics/texture_image.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include "world/world.h"
#include "utils/game_settings.h"

GraphicDebugger::GraphicDebugger(const std::shared_ptr<FrameGraph>& in_framegraph)
    : framegraph(in_framegraph) {
    window_name = "graphic debugger";

    fill_nodes(in_framegraph->get_root());
    node_at_index = std::vector<std::vector<std::shared_ptr<RenderPass>>>(node_map.size());
    update_node_pos(in_framegraph->get_root(), 0);
    int layer_count = 0;
    for (const auto& node : node_at_index) {
        if (!node.empty())
            layer_count++;
    }

    for (const auto& node : node_at_index) {
        float pos_y = 0;
        for (const auto& rt : node)
            pos_y -= rt->get_all_render_targets().size() / 2.f;
        for (const auto& rt : node) {
            node_map[rt].draw_y      = pos_y;
            node_map[rt].layer_count = layer_count;
            pos_y += rt->get_all_render_targets().size();
        }
    }
}

void GraphicDebugger::draw() {
    if (ImGui::BeginTabBar("GraphicSettingsTab")) {
        if (ImGui::BeginTabItem("Framegraph visualizer")) {

            if (ImGui::BeginCombo("frame graph", framegraph ? framegraph->name.c_str() : "None")) {
                for (const auto& item : FrameGraph::registry())
                    if (ImGui::MenuItem(item->name.c_str()))
                        framegraph = item;
                ImGui::EndCombo();
            }

            if (ImGui::BeginChild("frame graph")) {
                const auto win_min = ImGui::GetWindowPos();
                const auto win_max = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y);

                const auto io = ImGui::GetIO();
                if (io.MouseWheel && ImGui::IsMouseHoveringRect(win_min, win_max)) {
                    const float zoom     = io.MouseWheel * zoom_value * 0.1f;
                    ImVec2      percents = ImVec2((ImGui::GetMousePos().x - ImGui::GetWindowPos().x) / ImGui::GetWindowSize().x,
                                                  (ImGui::GetMousePos().y - ImGui::GetWindowPos().y) / ImGui::GetWindowSize().y - 0.5f);

                    // Remap percent range
                    percents = ImVec2(std::lerp(-zoom_center.x / ImGui::GetWindowSize().x / zoom_value,
                                                (-zoom_center.x + ImGui::GetWindowSize().x) / ImGui::GetWindowSize().x / zoom_value, percents.x),
                                      std::lerp(-zoom_center.y / ImGui::GetWindowSize().y / zoom_value,
                                                (-zoom_center.y + ImGui::GetWindowSize().y) / ImGui::GetWindowSize().y / zoom_value, percents.y));

                    zoom_value  = std::max(0.5f, zoom_value + zoom);
                    zoom_center = ImVec2(zoom_center.x - ImGui::GetWindowSize().x * zoom * percents.x, zoom_center.y - ImGui::GetWindowSize().y * zoom * percents.y);
                }

                ImGui::PushClipRect(win_min, win_max, true);
                ImGui::GetForegroundDrawList()->PushClipRect(win_min, win_max, true);
                const float res_ratio = framegraph->get_root()->get_width() / static_cast<float>(framegraph->get_root()->get_height());
                for (auto& node : node_map) {
                    node.second.draw_node(res_ratio, node_map, zoom_center, zoom_value);
                }
                ImGui::GetForegroundDrawList()->PopClipRect();
                ImGui::PopClipRect();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings")) {
            if (ImGui::Checkbox("Enable VSync", &GameSettings::get().v_sync))
                glfwSwapInterval(GameSettings::get().v_sync ? 1 : 0);

            ImGui::Checkbox("Wireframe", &GameSettings::get().wireframe);
            ImGui::Checkbox("Screen Space Reflections", &GameSettings::get().screen_space_reflections);

            ImGui::SliderFloat("Bloom intensity", &GameSettings::get().bloom_intensity, 0, 3);
            ImGui::SliderFloat("Exposure", &GameSettings::get().exposure, 0.1f, 4);
            ImGui::SliderFloat("Gamma", &GameSettings::get().gamma, 0.5f, 4);

            ImGui::DragInt("Framerate limit", &GameSettings::get().max_fps);
            if (GameSettings::get().max_fps < 0)
                GameSettings::get().max_fps = 0;

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

}

void GraphicDebugger::fill_nodes(const std::shared_ptr<RenderPass>& rp) {

    if (node_map.contains(rp))
        return;

    for (const auto& dep : rp->get_dependencies()) {
        fill_nodes(dep);
    }

    std::vector<Node*> parents;
    for (const auto& parent : rp->get_dependencies()) {
        parents.emplace_back(&node_map[parent]);
    }

    node_map[rp] = Node{.render_pass = rp, .parents = parents};

}

void GraphicDebugger::update_node_pos(const std::shared_ptr<RenderPass>& rp, int pos) {
    auto& node = node_map.find(rp)->second;

    if (pos < node.index_x)
        return;

    auto& old_pos = node_at_index[node.index_x];
    auto& new_pos = node_at_index[pos];

    const auto removed_item = std::find(old_pos.begin(), old_pos.end(), rp);
    if (removed_item != old_pos.end())
        old_pos.erase(removed_item);
    new_pos.emplace_back(rp);
    node.index_x = pos;

    for (const auto& child : node.parents)
        update_node_pos(child->render_pass, pos + 1);
}

void GraphicDebugger::Node::draw_node(float res_ratio, std::unordered_map<std::shared_ptr<RenderPass>, Node>& node_map, const ImVec2& center, float zoom) {

    const auto& min          = ImVec2(ImGui::GetWindowPos().x + center.x, ImGui::GetWindowPos().y + center.y);
    const float windows_size = ImGui::GetWindowSize().x * zoom;

    const ImVec2 item_size  = ImVec2(windows_size / layer_count, windows_size / layer_count / res_ratio);
    const ImVec2 margin     = ImVec2(item_size.x / 20, item_size.x / 20 / res_ratio);
    const ImVec2 padding    = ImVec2(margin.x + 2, margin.y + 2);
    const ImVec2 draw_start = ImVec2(min.x, min.y + ImGui::GetWindowSize().y / 2);

    const auto& render_targets = render_pass->get_all_render_targets();

    const auto get_min = [&](int x, float y) -> ImVec2 {
        return {
            windows_size + min.x - (x + 1) * item_size.x,
            draw_start.y + y * item_size.y
        };
    };
    const auto get_max = [&](int x, float y, size_t rt_count) -> ImVec2 {
        return {
            windows_size + min.x - (x + 1) * item_size.x + item_size.x,
            draw_start.y + y * item_size.y + item_size.y + (rt_count - 1) * item_size.y
        };
    };

    const ImVec2 p_min = get_min(index_x, draw_y);
    const ImVec2 p_max = get_max(index_x, draw_y, render_targets.size());

    const bool hover = ImGui::IsMouseHoveringRect(p_min, p_max);

    const auto& draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(
        ImVec2(p_min.x + margin.x, p_min.y + margin.y),
        ImVec2(p_max.x - margin.x, p_max.y - margin.y),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1))
        );
    draw_list->AddRect(
        ImVec2(p_min.x + margin.x, p_min.y + margin.y),
        ImVec2(p_max.x - margin.x, p_max.y - margin.y),
        ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 0, hover ? 1 : 0.4f))
        );

    for (size_t index = 0; index < render_targets.size(); ++index) {
        draw_target(
            render_targets[index],
            ImVec2(p_min.x + margin.x, p_min.y + index * item_size.y + margin.y),
            ImVec2(p_max.x - margin.x, p_min.y + (index + 1) * item_size.y - margin.y)
            );
    }
    const std::string text      = render_pass->name;
    const auto        text_size = ImGui::CalcTextSize(text.c_str());

    if (text_size.x < (p_max.x - p_min.x - padding.x * 2) * 0.75f)
        draw_list->AddText(ImVec2(p_max.x - text_size.x - 2 - padding.x, p_min.y + padding.y + 2), ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), text.c_str());

    if (hover) {

        ImGui::BeginTooltip();

        ImGui::Text("Render pass [%s]", render_pass->name.c_str());
        ImGui::Text("Size [%dx%d]", render_pass->get_width(), render_pass->get_height());
        ImGui::Text("Attachments : ");
        ImGui::SameLine();
        ImGui::BeginGroup();
        for (const auto& dep : render_pass->get_all_render_targets()) {
            ImGui::Text("%s | format = %s", dep->name.c_str(), image_format_to_string(dep->internal_format()).c_str());
        }
        ImGui::EndGroup();
        ImGui::EndTooltip();

        const auto fg_draw_list = ImGui::GetForegroundDrawList();
        for (const auto& dep : render_pass->get_dependencies()) {
            const auto& node     = node_map[dep];
            int         distance = abs(node.index_x - index_x);
            ImVec2      end_min  = get_min(node.index_x, node.draw_y);
            ImVec2      end_max  = get_max(node.index_x, node.draw_y, node.render_pass->get_all_render_targets().size());
            fg_draw_list->AddBezierCurve(ImVec2((p_min.x + p_max.x) / 2, p_min.y + margin.y),
                                         ImVec2((p_min.x + p_max.x) / 2, std::min(p_min.y, end_min.y) - distance * 10 * zoom),
                                         ImVec2((end_min.x + end_max.x) / 2, std::min(p_min.y, end_min.y) - distance * 10 * zoom),
                                         ImVec2((end_min.x + end_max.x) / 2, end_min.y + margin.y),
                                         ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 0, 1)), 2);

            std::string text;
            for (const auto& bp : render_pass->bind_point_names(dep)) {
                text += bp + "\n";
            }

            const auto text_size = ImGui::CalcTextSize(text.c_str());

            if (ImGui::GetWindowPos().y < ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - text_size.y) {
                    ImVec2 text_pos = ImVec2(std::max(ImGui::GetWindowPos().x, (end_min.x + end_max.x) / 2 - text_size.x / 2),
                                             std::clamp(end_min.y + margin.y, ImGui::GetWindowPos().y, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - text_size.y));

                    fg_draw_list->AddRectFilled(ImVec2(text_pos.x - 3, text_pos.y - 3), ImVec2(text_pos.x + text_size.x + 3, text_pos.y + text_size.y + 3),
                                                ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.8f)));
                    fg_draw_list->AddRect(ImVec2(text_pos.x - 3, text_pos.y - 3), ImVec2(text_pos.x + text_size.x + 3, text_pos.y + text_size.y + 3),
                                          ImGui::ColorConvertFloat4ToU32(ImVec4(0, 1, 1, 0.8f)));
                    fg_draw_list->AddText(text_pos, ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), text.c_str());
                }
        }
    }

}

void GraphicDebugger::Node::draw_target(const std::shared_ptr<Texture2D>& target, ImVec2 min, ImVec2 max) {

    ImGui::PushClipRect(min, max, true);
    const auto& draw_list = ImGui::GetWindowDrawList();
    draw_list->AddImage(reinterpret_cast<ImTextureID>(static_cast<size_t>(target->id())), min, max, ImVec2(0, 1), ImVec2(1, 0));

    std::string text = target->name + "\n" + std::to_string(target->width()) + " x " + std::to_string(target->height()) + "\n" + image_format_to_string(target->internal_format());
    const auto  text_size = ImGui::CalcTextSize(text.c_str());

    if (text_size.x < (max.x - min.x) * 0.5f)
        draw_list->AddText(ImVec2(min.x + 2, max.y - text_size.y - 2), ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), text.c_str());

    ImGui::PopClipRect();
}
