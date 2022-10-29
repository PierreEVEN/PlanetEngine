#pragma once
#include "ui.h"

#include <imgui.h>
#include <unordered_map>
#include <vector>

class Texture2D;
class RenderPass;
class FrameGraph;

class GraphicDebugger : public ImGuiWindow {
public:
    GraphicDebugger(const std::shared_ptr<FrameGraph>& framegraph);

    void draw() override;


private:
    void                                                  fill_nodes(const std::shared_ptr<RenderPass>& rp);
    void                                                  update_node_pos(const std::shared_ptr<RenderPass>& rp, int pos);
    std::vector<std::vector<std::shared_ptr<RenderPass>>> node_at_index;

    struct Node {
        int                         index_x = 0;
        float                       draw_y  = 0;
        std::shared_ptr<RenderPass> render_pass;
        std::vector<Node*>          parents;
        void                        draw_node(float res_ratio, std::unordered_map<std::shared_ptr<RenderPass>, Node>& node_map, const ImVec2& center, float zoom);
        int                         layer_count = 0;
        void                        draw_target(const std::shared_ptr<Texture2D>& target, ImVec2 min, ImVec2 max);
    };

    ImVec2                                                zoom_center = ImVec2(0, 0);
    float                                                 zoom_value  = 1;
    float                                                 current_pos = 0;
    std::unordered_map<std::shared_ptr<RenderPass>, Node> node_map;
    std::shared_ptr<FrameGraph>                           framegraph;
};
