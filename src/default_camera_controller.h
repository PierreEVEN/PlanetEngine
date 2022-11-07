#pragma once
#include <memory>

#include "world/scene_component.h"

class Camera;
struct GLFWwindow;

class DefaultCameraController : public SceneComponent
{
public:
	DefaultCameraController();
	~DefaultCameraController();

	void process_key(GLFWwindow* window, int key, int scan_code, int action, int mode);
	void process_mouse_input(GLFWwindow* window, double x_pos, double y_pos);
	void process_mouse_wheel(GLFWwindow* window, double x_pos, double y_pos);

	void tick(double delta_time) override;

	void teleport_to(const Eigen::Vector3d& new_location);

	virtual Class get_class() override { return Class(this); }
private:

	void teleport_to_ground();

	std::shared_ptr<Camera> get_camera() const;

	bool set_last_mouse = false;
	double last_mouse_x = 0;
	double last_mouse_y = 0;
	double input_add_x = 0;
	double input_add_y = 0;
	double input_add_z = 0;
	double input_sub_x = 0;
	double input_sub_y = 0;
	double input_sub_z = 0;
	double input_add_roll = 0;
	double input_sub_roll = 0;
	double target_roll = 0;
	Eigen::Vector3d camera_desired_position = Eigen::Vector3d(0, 0, 0);
	double movement_speed = 100;
	bool capture_input = false;
};
