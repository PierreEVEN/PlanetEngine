#pragma once
#include "ui.h"

class FrameGraph;

class GraphicDebugger : public ImGuiWindow {
public:
    GraphicDebugger(const std::shared_ptr<FrameGraph>& framegraph);

    void draw() override;


private:
    std::shared_ptr<FrameGraph> framegraph;
};
