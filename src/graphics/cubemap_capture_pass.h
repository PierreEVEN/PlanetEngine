#pragma once
#include "render_pass.h"

class CubemapCapturePass : public RenderPass {

public:
    static std::shared_ptr<CubemapCapturePass> create(std::string name, uint32_t width) {
        return std::shared_ptr<CubemapCapturePass>(new CubemapCapturePass(name, width));
    }

    void render(bool to_back_buffer) override;
    virtual ~CubemapCapturePass();
private:
    CubemapCapturePass(std::string name, uint32_t width);
};
