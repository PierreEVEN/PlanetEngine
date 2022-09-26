#include "default_camera_controller.h"

#include <GLFW/glfw3.h>

#include "camera.h"

DefaultCameraController::DefaultCameraController(const std::shared_ptr<Camera>& in_camera) : camera(in_camera)
{
}

void DefaultCameraController::process_key(int key, int scan_code, int action, int mode)
{
	constexpr double speed = 0.25;

	if (action == GLFW_RELEASE)
		return;

	const auto pos = camera->get_local_position();
	
	switch (key)
	{
	case GLFW_KEY_W:
		camera->set_local_position(pos + camera->world_forward() * speed);
		break;
	case GLFW_KEY_A:
		camera->set_local_position(pos + camera->world_right() * speed);
		break;
	case GLFW_KEY_S:
		camera->set_local_position(pos - camera->world_forward() * speed);
		break;
	case GLFW_KEY_D:
		camera->set_local_position(pos - camera->world_right() * speed);
		break;
	case GLFW_KEY_SPACE:
		camera->set_local_position(pos + camera->world_up() * speed);
		break;
	case GLFW_KEY_LEFT_SHIFT:
		camera->set_local_position(pos - camera->world_up() * speed);
		break;
	default:
		break;
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
	camera->set_pitch(std::clamp(camera->get_pitch() + (y_pos - last_mouse_y) * speed, -M_PI / 2, M_PI/2));

	last_mouse_x = x_pos;
	last_mouse_y = y_pos;
}
