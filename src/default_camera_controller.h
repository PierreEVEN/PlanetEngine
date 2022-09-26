#pragma once
#include <memory>

class Camera;

class DefaultCameraController
{
public:
	DefaultCameraController(const std::shared_ptr<Camera>& camera);

	void process_key(int key, int scan_code, int action, int mode);
	void process_mouse_input(double x_pos, double y_pos);

private:
	bool set_last_mouse = false;
	double last_mouse_x;
	double last_mouse_y;
	std::shared_ptr<Camera> camera;
};
