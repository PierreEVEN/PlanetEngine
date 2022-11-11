#include "cubemap_capture_pass.h"

#include "utils/profiler.h"

#include <GL/gl3w.h>

void CubemapCapturePass::render(bool to_back_buffer) {
    if (!pre_render())
        return;
    {
        STAT_FRAME("Bind render pass [" + name + "]");
        bind(to_back_buffer);
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GREATER);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glFrontFace(GL_CCW);
        glClearColor(0, 0, 0, 0);
        glClearDepth(0.0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }
    STAT_FRAME("Draw render pass [" + name + "]");
    on_draw.execute();
}

CubemapCapturePass::~CubemapCapturePass() {
}

CubemapCapturePass::CubemapCapturePass(std::string name, uint32_t width)
    : RenderPass(name, width, width) {

}
