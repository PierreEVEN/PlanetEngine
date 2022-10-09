#include "default_camera_controller.h"

#include <iostream>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "engine/engine.h"
#include "utils/maths.h"
#include "utils/profiler.h"

DefaultCameraController::DefaultCameraController(const std::shared_ptr<Camera>& in_camera) : camera(in_camera),
                                                                                             camera_desired_position(0, 0, 0)
{
	Engine::get().on_key_down.add_object(this, &DefaultCameraController::process_key);
	Engine::get().on_mouse_moved.add_object(this, &DefaultCameraController::process_mouse_input);
	Engine::get().on_mouse_scroll.add_object(this, &DefaultCameraController::process_mouse_wheel);
}

DefaultCameraController::~DefaultCameraController()
{
	Engine::get().on_key_down.clear_object(this);
	Engine::get().on_mouse_moved.clear_object(this);
	Engine::get().on_mouse_scroll.clear_object(this);
}

void DefaultCameraController::process_key(GLFWwindow* window, int key, int scan_code, int action, int mode)
{
	if (action == GLFW_PRESS)
	{
		if (!capture_input && key != GLFW_KEY_ESCAPE)
			return;

		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			capture_input = !capture_input;
			glfwSetInputMode(window, GLFW_CURSOR, capture_input ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
			set_last_mouse = false;
			break;
		case GLFW_KEY_W:
			input_add_x = 1;
			break;
		case GLFW_KEY_A:
			input_add_y = 1;
			break;
		case GLFW_KEY_S:
			input_sub_x = 1;
			break;
		case GLFW_KEY_D:
			input_sub_y = 1;
			break;
		case GLFW_KEY_SPACE:
			input_add_z = 1;
			break;
		case GLFW_KEY_LEFT_SHIFT:
			input_sub_z = 1;
			break;
		default:
			break;
		}
	}
	else if (action == GLFW_RELEASE)
	{
		switch (key)
		{
		case GLFW_KEY_W:
			input_add_x = 0;
			break;
		case GLFW_KEY_A:
			input_add_y = 0;
			break;
		case GLFW_KEY_S:
			input_sub_x = 0;
			break;
		case GLFW_KEY_D:
			input_sub_y = 0;
			break;
		case GLFW_KEY_SPACE:
			input_add_z = 0;
			break;
		case GLFW_KEY_LEFT_SHIFT:
			input_sub_z = 0;
			break;
		default:
			break;
		}
	}
}

void DefaultCameraController::process_mouse_input(GLFWwindow* window, double x_pos, double y_pos)
{
	if (!capture_input)
		return;
	if (!set_last_mouse)
	{
		set_last_mouse = true;
		last_mouse_x = x_pos;
		last_mouse_y = y_pos;
	}
	constexpr double speed = 0.01;

	camera->set_yaw(camera->get_yaw() + (x_pos - last_mouse_x) * speed);
	camera->set_pitch(std::clamp(camera->get_pitch() + (y_pos - last_mouse_y) * speed, -M_PI / 2, M_PI / 2));

	last_mouse_x = x_pos;
	last_mouse_y = y_pos;
}

void DefaultCameraController::process_mouse_wheel(GLFWwindow* window, double x_pos, double y_pos)
{
	if (!capture_input)
		return;
	movement_speed *= y_pos / 3 + 1;
	if (movement_speed < 0.001)
		movement_speed = 0.001;
}

void DefaultCameraController::tick(double delta_time)
{
	STAT_DURATION("CameraController_update");
	camera_desired_position += (camera->world_forward() * (input_add_x - input_sub_x) + camera->world_right() * (
		input_add_y - input_sub_y) + camera->world_up() * (input_add_z - input_sub_z)) * movement_speed * delta_time;
	camera->set_local_position(Maths::lerp(camera->get_local_position(), camera_desired_position, 15 * delta_time));
}

void DefaultCameraController::teleport_to(const Eigen::Vector3d& new_location)
{
	camera->set_local_position(new_location);
	camera_desired_position = new_location;
}
