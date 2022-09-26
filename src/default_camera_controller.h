#pragma once
#include <memory>

#include <Eigen/Eigen>

class Camera;

class DefaultCameraController
{
public:
	DefaultCameraController(const std::shared_ptr<Camera>& camera);

	void process_key(int key, int scan_code, int action, int mode);
	void process_mouse_input(double x_pos, double y_pos);
	void process_mouse_wheel(double x_pos, double y_pos);

	void tick(double delta_time);

private:
	bool set_last_mouse = false;
	double last_mouse_x = 0;
	double last_mouse_y = 0;
	std::shared_ptr<Camera> camera;
	double input_add_x = 0;
	double input_add_y = 0;
	double input_add_z = 0;
	double input_sub_x = 0;
	double input_sub_y = 0;
	double input_sub_z = 0;
	Eigen::Vector3d camera_desired_position = Eigen::Vector3d(0, 0, 0);
	double movement_speed = 0.25;
};
