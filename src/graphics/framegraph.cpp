#include "framegraph.h"

#include "render_pass.h"
#include "utils/profiler.h"

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
