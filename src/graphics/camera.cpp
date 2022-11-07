#include "camera.h"

#include "uniform_buffer.h"

#include <imgui.h>
#include <iostream>
#include <GLFW/glfw3.h>

static std::shared_ptr<UniformBuffer> camera_ubo;

struct WorldDataStructure {
    alignas(16) Eigen::Matrix4f proj_matrix;
    alignas(16) Eigen::Matrix4f view_matrix;
    alignas(16) Eigen::Matrix4f vp_matrix;
    alignas(16) Eigen::Matrix4f proj_matrix_inv;
    alignas(16) Eigen::Matrix4f view_matrix_inv;
    alignas(16) Eigen::Matrix4f vp_matrix_inv;
    alignas(16) float           world_time;
    alignas(16) Eigen::Vector3f camera_pos;
    alignas(16) Eigen::Vector3f camera_forward;
};

Camera::Camera()
    : SceneComponent("camera"), res({800, 600}), camera_fov(45), camera_near(0.1), pitch(0), yaw(0), roll(0) {
    update_rotation();
    if (!camera_ubo)
        camera_ubo = UniformBuffer::create("camera UBO");
}

Eigen::Matrix4d Camera::reversed_z_projection_matrix() const {
    if (res.x() <= 0 || res.y() <= 0)
        return Eigen::Matrix4d::Identity();

    const auto tan_h_fov    = 1.0 / std::tan(camera_fov / 360 * M_PI);
    const auto aspect_ratio = res.x() / res.y();

    Eigen::Matrix4d m;
    m << tan_h_fov / aspect_ratio, 0, 0, 0,
        0, tan_h_fov, 0, 0,
        0, 0, 0.0, camera_near,
        0, 0, -1, 0;

    return m;
}

Eigen::Matrix4d Camera::view_matrix() {
    // There is no translation in view matrix to keep it at the center of the scene. (the translation is done in model matrix)
    const auto corr_z    = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ());
    const auto corr_y    = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitY());
    auto       mat_trans = Eigen::Affine3d::Identity();
    mat_trans.rotate(Eigen::Quaterniond(corr_z * corr_y));
    mat_trans.rotate(get_world_rotation().inverse());
    return mat_trans.matrix();
}

Eigen::Quaterniond Camera::get_world_rotation() {

    if (get_parent())
        return Eigen::Quaterniond(get_local_rotation() * get_parent()->get_world_rotation());

    return SceneComponent::get_local_rotation();
}

Eigen::Vector3d Camera::get_world_position() const {
    if (get_parent())
        return get_parent()->get_world_position() + get_local_position();
    return SceneComponent::get_world_position();
}

void Camera::set_pitch(double in_pitch) {
    pitch = in_pitch;
    update_rotation();
}

void Camera::set_yaw(double in_yaw) {
    yaw = in_yaw;
    update_rotation();
}

void Camera::set_roll(double in_roll) {
    roll = in_roll;
    update_rotation();
}

void Camera::tick(double delta_time) {
    SceneComponent::tick(delta_time);
}

void Camera::draw_ui() {
    SceneComponent::draw_ui();
    auto fov = static_cast<float>(camera_fov);
    if (ImGui::SliderFloat("FOV", &fov, 1, 179))
        camera_fov = fov;
    auto near = static_cast<float>(camera_near);
    if (ImGui::DragFloat("Z near", &near, 0.01f))
        camera_near = near;
    if (camera_near < 0.001)
        camera_near = 0.001;
}

void Camera::use() {
    const auto               proj_matrix = reversed_z_projection_matrix();
    const auto               view_mat = view_matrix();
    const auto               pv_matrix   = proj_matrix * view_mat;
    const WorldDataStructure world_data  = {
        .proj_matrix = proj_matrix.cast<float>(),
        .view_matrix = view_mat.cast<float>(),
        .vp_matrix = pv_matrix.cast<float>(),
        .proj_matrix_inv = proj_matrix.cast<float>().inverse(),
        .view_matrix_inv = view_mat.cast<float>().inverse(),
        .vp_matrix_inv = pv_matrix.cast<float>().inverse(),
        .world_time = static_cast<float>(glfwGetTime()),
        .camera_pos = get_world_position().cast<float>(),
        .camera_forward = world_forward().cast<float>(),
    };
    camera_ubo->set_data(world_data);
}

void Camera::update_rotation() {
    const auto p = Eigen::AngleAxisd(pitch, Eigen::Vector3d::UnitY());
    const auto y = Eigen::AngleAxisd(-yaw, Eigen::Vector3d::UnitZ());
    const auto r = Eigen::AngleAxisd(roll, Eigen::Vector3d::UnitX());
    set_local_rotation(y * r * p);
}
