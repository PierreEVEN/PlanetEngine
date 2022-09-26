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


static Eigen::Matrix4d test_look_at_matrix(const Eigen::Vector3d& cameraLocation, const Eigen::Vector3d& viewTarget,
                                        const Eigen::Vector3d& upVector)
{
	Eigen::Vector3d const f((viewTarget - cameraLocation).normalized());
	Eigen::Vector3d const s(f.cross(upVector).normalized());
	Eigen::Vector3d const u(s.cross(f));
	Eigen::Matrix4d Result = Eigen::Matrix4d::Identity();
	Result(0, 0) = s.x();
	Result(1, 0) = s.y();
	Result(2, 0) = s.z();
	Result(0, 1) = u.x();
	Result(1, 1) = u.y();
	Result(2, 1) = u.z();
	Result(0, 2) = -f.x();
	Result(1, 2) = -f.y();
	Result(2, 2) = -f.z();
	Result(3, 0) = -s.dot(cameraLocation);
	Result(3, 1) = -u.dot(cameraLocation);
	Result(3, 2) = f.dot(cameraLocation);
	return Result;
}

Eigen::Matrix4d Camera::view_matrix()
{
	//return test_look_at_matrix(get_local_position(), get_local_position() + world_forward(), Eigen::Vector3d::UnitZ());
	
	const auto f = world_forward();
	const auto r = world_right();
	const auto u = world_up();

	const auto wp = get_world_position();

	Eigen::Matrix4d m;
	m <<
		r.x(), u.x(), -f.x(), -r.dot(wp),
		r.y(), u.y(), -f.y(), -u.dot(wp),
		r.z(), u.z(), -f.z(), f.dot(wp),
		0, 0, 0, 1;
	return m;

	const auto corr_z = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ());
	const auto corr_y = Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitY());
	auto transform = get_world_transform();
	transform.rotate(Eigen::Quaterniond(corr_z * corr_y));

	return transform.matrix();
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
	const auto r = Eigen::AngleAxisd(c, Eigen::Vector3d::UnitX());
	const auto p = Eigen::AngleAxisd(a, Eigen::Vector3d::UnitY());
	const auto y = Eigen::AngleAxisd(b, Eigen::Vector3d::UnitZ());

	set_local_rotation(y * p * r);
}
