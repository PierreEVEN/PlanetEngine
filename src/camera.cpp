#include "camera.h"

Camera::Camera() : SceneComponent("camera"), res({800, 600}), camera_fov(45), camera_near(0.1), pitch(0), yaw(0)
{
	update_rotation();
}

/*
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
*/

Eigen::Matrix4d Camera::reversed_z_projection_matrix() const
{
	const auto tan_h_fov = 1.0 / std::tan(camera_fov / 2.0);
	const auto aspect_ratio = res.x() / res.y();

	Eigen::Matrix4d m;
	m << tan_h_fov / aspect_ratio, 0, 0, 0,
		0, tan_h_fov, 0, 0,
		0, 0, 0.0, camera_near,
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
}

void Camera::update_rotation()
{
	const auto p = Eigen::AngleAxisd(pitch, Eigen::Vector3d::UnitY());
	const auto y = Eigen::AngleAxisd(-yaw, Eigen::Vector3d::UnitZ());
	set_local_rotation(y * p);
}
