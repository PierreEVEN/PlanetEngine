#pragma once

#include "world/scene_component.h"

class Camera : public SceneComponent
{
public:
	Camera();

	//Eigen::Matrix4d projection_matrix() const;
	Eigen::Matrix4d reversed_z_projection_matrix() const;
	Eigen::Matrix4d view_matrix();

	double& field_of_view()
	{
		return camera_fov;
	}

	double& z_near()
	{
		return camera_near;
	}

	Eigen::Vector2d& viewport_res()
	{
		return res;
	}

	void set_pitch(double pitch);
	void set_yaw(double yaw);
	void set_roll(double roll);

	[[nodiscard]] double get_pitch() const
	{
		return pitch;
	}

	[[nodiscard]] double get_yaw() const
	{
		return yaw;
	}

	[[nodiscard]] double get_roll() const
	{
		return roll;
	}

	void tick(double delta_time) override;
	void draw_ui() override;

	[[nodiscard]] Eigen::Vector3d get_world_position() const override
	{
		if (get_parent())
			return get_parent()->get_world_transform().translation() + get_local_position(); //@TODO : fix rotation
		return get_local_position();
	}
private:

	void update_rotation();

	Eigen::Vector2d res;
	double camera_fov;
	double camera_near;

	double pitch;
	double yaw;
	double roll;
};
