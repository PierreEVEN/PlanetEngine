
#include "framegraph.h"

#include "render_pass.h"

void FrameGraph::render() {
    if (width == 0 || height == 0)
        return;

    if (resized) {
        root->resize(width, height);
        resized = false;
    }
	root->reset();
	root->render(false);
}

void FrameGraph::resize(uint32_t in_width, uint32_t in_height) {
    if (in_height == height && in_width == width || in_height == 0 || in_width == 0)
        return;
    width   = in_width;
    height  = in_height;
    resized = true;
}
