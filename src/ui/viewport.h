#pragma once
#include "ui.h"
#include <Eigen/Dense>


class Texture2D;

class Viewport : public ImGuiWindow {
public:
    Viewport(const std::shared_ptr<Texture2D>& framebuffer_texture);
    void draw() override;

    [[nodiscard]] uint32_t width() const { return viewport_width; };
    [[nodiscard]] uint32_t height() const { return viewport_height; }

private:
    uint32_t                   viewport_width  = 0;
    uint32_t                   viewport_height = 0;
    std::shared_ptr<Texture2D> framebuffer_texture;
};
