#include "default_camera_controller.h"

#include <iostream>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "utils/maths.h"

DefaultCameraController::DefaultCameraController(const std::shared_ptr<Camera>& in_camera) : camera(in_camera)
{
}

void DefaultCameraController::process_key(int key, int scan_code, int action, int mode)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
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

void DefaultCameraController::process_mouse_input(double x_pos, double y_pos)
{
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

void DefaultCameraController::process_mouse_wheel(double x_pos, double y_pos)
{
	movement_speed *= y_pos / 3 + 1;
}

void DefaultCameraController::tick(double delta_time)
{
	camera_desired_position += (camera->world_forward() * (input_add_x - input_sub_x) + camera->world_right() * (input_add_y - input_sub_y) + camera->world_up() * (input_add_z - input_sub_z)) * movement_speed;
	camera->set_local_position(Maths::lerp(camera->get_local_position(), camera_desired_position, 20 * delta_time));
}
