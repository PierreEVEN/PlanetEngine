#pragma once
#include "ui.h"

#include <imgui.h>
#include <unordered_map>
#include <vector>

class RenderPass;
class FrameGraph;

class GraphicDebugger : public ImGuiWindow {
public:
    GraphicDebugger(const std::shared_ptr<FrameGraph>& framegraph);

    void draw() override;


private:
    void expand_node(const std::shared_ptr<RenderPass>& rp);

    struct Node {
        ImVec2                      position;
        ImVec2                      size;
        std::shared_ptr<RenderPass> render_pass;
        std::vector<Node*>          parents;
        void                        draw_node();
    };

    ImVec2                                                current_pos = ImVec2(0, 0);
    std::unordered_map<std::shared_ptr<RenderPass>, Node> nodes;
    std::shared_ptr<FrameGraph>                           framegraph;
};
