#include "default_camera_controller.h"

#include <iostream>
#include <GLFW/glfw3.h>

#include "engine/engine.h"
#include "graphics/camera.h"
#include "utils/maths.h"
#include "utils/profiler.h"
#include "world/planet.h"

DefaultCameraController::DefaultCameraController()
    : SceneComponent("camera controller"),
      camera_desired_position(0, 0, 0) {
    Engine::get().on_key_down.add_object(this, &DefaultCameraController::process_key);
    Engine::get().on_mouse_moved.add_object(this, &DefaultCameraController::process_mouse_input);
    Engine::get().on_mouse_scroll.add_object(this, &DefaultCameraController::process_mouse_wheel);
    set_tick_group(TickGroup::PrePhysic);
}

DefaultCameraController::~DefaultCameraController() {
    Engine::get().on_key_down.clear_object(this);
    Engine::get().on_mouse_moved.clear_object(this);
    Engine::get().on_mouse_scroll.clear_object(this);
}

void DefaultCameraController::process_key(GLFWwindow* window, int key, int scan_code, int action, int mode) {
    if (action == GLFW_PRESS) {
        if (!capture_input && key != GLFW_KEY_ESCAPE)
            return;

        switch (key) {
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
        case GLFW_KEY_Q:
            input_sub_roll = 1;
            break;
        case GLFW_KEY_E:
            input_add_roll = 1;
            break;
        case GLFW_KEY_F:
            teleport_to_ground();
            break;
        default:
            break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (key) {
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
        case GLFW_KEY_Q:
            input_sub_roll = 0;
            break;
        case GLFW_KEY_E:
            input_add_roll = 0;
            break;
        default:
            break;
        }
    }
}

void DefaultCameraController::process_mouse_input(GLFWwindow* window, double x_pos, double y_pos) {
    if (!capture_input)
        return;
    if (!set_last_mouse) {
        set_last_mouse = true;
        last_mouse_x   = x_pos;
        last_mouse_y   = y_pos;
    }
    constexpr double speed = 0.01;

    get_camera()->set_yaw(get_camera()->get_yaw() + (x_pos - last_mouse_x) * speed);
    get_camera()->set_pitch(std::clamp(get_camera()->get_pitch() + (y_pos - last_mouse_y) * speed, -M_PI / 2, M_PI / 2));

    last_mouse_x = x_pos;
    last_mouse_y = y_pos;
}

void DefaultCameraController::process_mouse_wheel(GLFWwindow* window, double x_pos, double y_pos) {
    if (!capture_input)
        return;
    movement_speed *= y_pos / 3 + 1;
    if (movement_speed < 0.1)
        movement_speed = 0.1;
}

void DefaultCameraController::tick(double delta_time) {
    STAT_FRAME("CameraController_update");

    if (delta_time > 1 / 20.0)
        delta_time = 1 / 20.0;

    target_roll = target_roll += (input_add_roll - input_sub_roll) * delta_time;
    get_camera()->set_roll(target_roll);
    camera_desired_position += (get_camera()->world_forward() * (input_add_x - input_sub_x) + get_camera()->world_right() * (
                                    input_add_y - input_sub_y) + get_camera()->world_up() * (input_add_z - input_sub_z)) * movement_speed * delta_time;
    
    if (get_parent()->get_class() == Class::of<Planet>()) {
        Planet* planet = static_cast<Planet*>(get_parent());
        if (camera_desired_position.norm() < get_camera()->get_local_position().normalized().norm() * (planet->get_radius() + 2.0)) {
            camera_desired_position = camera_desired_position.normalized() * (planet->get_radius() + 2.0);
        }
    }


    get_camera()->set_local_position(Maths::lerp(get_camera()->get_local_position(), camera_desired_position, 15 * delta_time));
}

void DefaultCameraController::teleport_to(const Eigen::Vector3d& new_location) {
    get_camera()->set_local_position(new_location);
    camera_desired_position = new_location;
}

void DefaultCameraController::teleport_to_ground() {
    if (get_parent()->get_class() == Class::of<Planet>()) {
        Planet* planet = static_cast<Planet*>(get_parent());

        camera_desired_position = get_camera()->get_local_position().normalized() * (planet->get_radius() + 2);
        movement_speed          = 100;
    }
}

std::shared_ptr<Camera> DefaultCameraController::get_camera() const {
    for (const auto& child : get_children())
        if (child->get_class() == Class::of<Camera>())
            return dynamic_pointer_cast<Camera>(child);
    return nullptr;
}
