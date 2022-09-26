#include "camera.h"

#include <imgui.h>
#include <iostream>

Camera::Camera(): res({800, 600}), camera_fov(45), camera_near(0.1), camera_far(10000), pitch(0), yaw(0)
{
	update_rotation();
}

float a = 0, b = 0, c = 0;

Eigen::Matrix4d Camera::projection_matrix() const
{
	const auto tan_h_fov = 1.0 / std::tan(camera_fov / 2.0);
	const auto aspect_ratio = res.x() / res.y();
	const auto range_inv = 1.0 / (camera_near - camera_far);

	Eigen::Matrix4d m;
	m << tan_h_fov / aspect_ratio, 0, 0, 0,
		0, tan_h_fov, 0, 0,
		0, 0, (camera_far + camera_near) * range_inv, 2.0 * camera_near * camera_far * range_inv,
		0, 0, -1, 0;

	return m;
}

Eigen::Matrix4d Camera::view_matrix()
{
	const auto corr_z = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ());
	const auto corr_y = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitY());
	auto mat_trans = Eigen::Affine3d::Identity();
	mat_trans.rotate(Eigen::Quaterniond(corr_z * corr_y));
	mat_trans.rotate(get_world_rotation().inverse());
	mat_trans.translate(-get_world_position());
	return mat_trans.matrix();
}

void Camera::set_pitch(double in_pitch)
{
	pitch = in_pitch;
	update_rotation();
}

void Camera::set_yaw(double in_yaw)
{
	yaw = in_yaw;
	update_rotation();
}

void Camera::tick(double delta_time)
{
	SceneComponent::tick(delta_time);

	const auto f = world_forward();
	const auto r = world_right();
	const auto up = world_up();

	ImGui::Text("forward ! x=%f, y=%f, z=%f", f.x(), f.y(), f.z());
	ImGui::Text("right ! x=%f, y=%f, z=%f", r.x(), r.y(), r.z());
	ImGui::Text("up ! x=%f, y=%f, z=%f", up.x(), up.y(), up.z());

	const auto wp = get_world_position();

	ImGui::Text("world_pos ! x=%f, y=%f, z=%f", wp.x(), wp.y(), wp.z());

	ImGui::SliderFloat("x", &a, -M_PI, M_PI);
	ImGui::SliderFloat("y", &b, -M_PI, M_PI);
	ImGui::SliderFloat("z", &c, -M_PI, M_PI);
}

void Camera::update_rotation()
{
	const auto p = Eigen::AngleAxisd(pitch, Eigen::Vector3d::UnitY());
	const auto y = Eigen::AngleAxisd(-yaw, Eigen::Vector3d::UnitZ());
	set_local_rotation(y * p);
}
