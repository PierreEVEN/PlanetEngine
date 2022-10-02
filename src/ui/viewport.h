#pragma once
#include "ui.h"
#include <Eigen/Eigen>

class Viewport : public ImGuiWindow
{
public:
	Viewport();
	virtual ~Viewport() override;
	void draw() override;
private:
	Eigen::Vector2i last_viewport_res;
	void on_fullscreen(bool fullscreen);
};
