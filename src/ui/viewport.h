#pragma once
#include "ui.h"
#include <Eigen/Dense>

namespace EZCOGL
{
	class TextureInterface;
}

class Viewport : public ImGuiWindow
{
public:
	Viewport(const std::shared_ptr<EZCOGL::TextureInterface>& framebuffer_texture);
	virtual ~Viewport() override;
	void draw() override;
private:
	Eigen::Vector2i last_viewport_res;
	void on_fullscreen(bool fullscreen);
	std::shared_ptr<EZCOGL::TextureInterface> framebuffer_texture;
};
