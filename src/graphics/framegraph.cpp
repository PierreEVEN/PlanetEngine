#include "framegraph.h"

#include "render_pass.h"
#include "utils/profiler.h"

static std::vector<std::shared_ptr<FrameGraph>> framegraph_list;

std::shared_ptr<FrameGraph> FrameGraph::create(std::string in_name, std::shared_ptr<RenderPass> in_root) {
    return framegraph_list.emplace_back(std::shared_ptr<FrameGraph>(new FrameGraph(std::move(in_name), std::move(in_root))));
}

void FrameGraph::render(bool to_back_buffer, uint32_t in_width, uint32_t in_height) {
    resize(in_width, in_height);

    STAT_FRAME("Render framegraph");
    if (width == 0 || height == 0)
        return;

    if (resized) {
        root->resize(width, height);
        resized = false;
    }
    {
        STAT_FRAME("Reset framegraph");
        root->reset();
    }

    root->render(to_back_buffer);
}

void FrameGraph::resize(uint32_t in_width, uint32_t in_height) {
    if (in_height == height && in_width == width || in_height == 0 || in_width == 0)
        return;

    STAT_FRAME("Resize framegraph to " + std::to_string(in_width) + " x " + std::to_string(height));
    width   = in_width;
    height  = in_height;
    resized = true;
}

const std::vector<std::shared_ptr<FrameGraph>>& FrameGraph::registry() { return framegraph_list; }

FrameGraph::~FrameGraph() {
    for (int64_t i = framegraph_list.size() - 1; i >= 0; --i)
        if (framegraph_list[i].get() == this)
            framegraph_list.erase(framegraph_list.begin() + i);
}
