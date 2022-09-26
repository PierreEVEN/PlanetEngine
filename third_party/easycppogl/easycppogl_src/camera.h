/*******************************************************************************
 * EasyCppOGL:   Copyright (C) 2019,                                            *
 * Sylvain Thery, IGG Group, ICube, University of Strasbourg, France            *
 *                                                                              *
 * This library is free software; you can redistribute it and/or modify it      *
 * under the terms of the GNU Lesser General Public License as published by the *
 * Free Software Foundation; either version 2.1 of the License, or (at your     *
 * option) any later version.                                                   *
 *                                                                              *
 * This library is distributed in the hope that it will be useful, but WITHOUT  *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
 * for more details.                                                            *
 *                                                                              *
 * You should have received a copy of the GNU Lesser General Public License     *
 * along with this library; if not, write to the Free Software Foundation,      *
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
 *                                                                              *
 * Contact information: thery@unistra.fr                                        *
 *******************************************************************************/

#ifndef EASY_CPP_OGL_CAMERA_H_
#define EASY_CPP_OGL_CAMERA_H_

#include <iostream>
#include <mframe.h>

namespace EZCOGL
{

class Camera : public MovingFrame
{
public:
	enum Type
	{
		PERSPECTIVE,
		ORTHOGRAPHIC
	};

	enum Mode
	{
		MANIPULATION,
		NAVIGATION
	};


private:
	Type type_;
	double field_of_view_;
	double asp_ratio_; // width/height
	mutable double znear_;
	mutable double zfar_;
	Eigen::Vector3d pivot_point_;
	double scene_radius_;
	double focal_dist_;
	mutable Eigen::Matrix4d projd_;
	mutable Eigen::Matrix4d mvd_;

	double nav_speed_;

	mutable uint32_t need_computing_;

	Eigen::Matrix4d perspective_d(double znear, double zfar) const;

	Eigen::Matrix4d ortho_d(double znear, double zfar) const;

	GLMat4 perspective(double znear, double zfar) const;

	GLMat4 ortho(double znear, double zfar) const;

	Mode navigation_mod_ = Mode::MANIPULATION;


public:
	inline Camera()
		: type_(PERSPECTIVE), field_of_view_(0.78), asp_ratio_(1.0), znear_(0), zfar_(0), nav_speed_(0.0),
		  need_computing_(0), scene_radius_(0.0), focal_dist_(0.0)
	{
	}

	inline bool is_navigation_mode() const
	{
		return navigation_mod_== Mode::NAVIGATION;
	}

	inline void set_mode(Mode mp)
	{
		if (navigation_mod_ == mp)
			return;
		if (mp == Mode::MANIPULATION)
			frame_ = Eigen::Translation3d(0.0, 0.0, focal_dist_) * frame_ * Eigen::Translation3d(pivot_point_); 
		else
			frame_ = Eigen::Translation3d(0.0, 0.0, -focal_dist_) *  frame_ * Eigen::Translation3d(-pivot_point_); 
		navigation_mod_ = mp;
	}

	inline double width() const
	{
		return (asp_ratio_ > 1.0) ? asp_ratio_ : 1.0;
	}

	inline double height() const
	{
		return (asp_ratio_ > 1.0) ? 1.0 : 1.0 / asp_ratio_;
	}

	inline void set_type(Type type)
	{
		type_ = type;
		need_computing_ = 3;
	}

	inline void set_field_of_view(double fov)
	{
		field_of_view_ = fov;
		focal_dist_ = scene_radius_ / std::tan(field_of_view_ / 2.0);
		need_computing_ = 3;
	}

	inline double field_of_view()
	{
		return field_of_view_;
	}

	inline float zcam() const
	{
		return float(focal_dist_ / scene_radius_);
	}

	inline void set_aspect_ratio(double aspect)
	{
		asp_ratio_ = aspect;
		need_computing_ = 3;
	}

	inline void set_scene_radius(double radius)
	{
		scene_radius_ = radius;
		focal_dist_ = scene_radius_ / std::tan(field_of_view_ / 2.0);
		need_computing_ = 3;
	}

	inline void change_pivot_point(const Eigen::Vector3d& piv)
	{
		//		pivot_shift_ = piv-pivot_point_;
		//		frame_ *= Eigen::Translation3d(pivot_shift_);

		frame_ *= Eigen::Translation3d(piv - pivot_point_);
		//		pivot_point_ = piv;
		need_computing_ = 3;
	}

	inline void set_pivot_point(const Eigen::Vector3f& piv)
	{
		pivot_point_ = piv.cast<double>();
	}

	inline void center_scene()
	{
		this->frame_.matrix().block<3, 1>(0, 3).setZero();
		need_computing_ = 3;
	}

	inline void show_entire_scene()
	{
		this->frame_.matrix().block<3, 1>(0, 3).setZero();
		need_computing_ = 3;
	}

	inline void reset()
	{
		this->frame_ = Eigen::Affine3d::Identity();
		this->spin_ = Eigen::Affine3d::Identity();
		need_computing_ = 3;
	}

	inline double scene_radius() const
	{
		return scene_radius_;
	}

	// alias scene_center
	inline const Eigen::Vector3d& pivot_point() const
	{
		return pivot_point_;
	}

	inline const Eigen::Vector3f pivot_point_f() const
	{
		return pivot_point_.cast<float>();
	}

	Eigen::Matrix4d get_projection_matrix_d() const;

	Eigen::Matrix4d get_view_matrix_d() const;

	inline GLMat4 get_projection_matrix() const
	{
		return get_projection_matrix_d().cast<float>();
	}

	inline GLMat4 get_view_matrix() const
	{
		return get_view_matrix_d().cast<float>();
	}

	inline float focal_dist() const
	{
		return float(focal_dist_);
	}

	inline float z_near() const
	{
		return float(znear_);
	}

	inline float z_far() const
	{
		return float(zfar_);
	}

	void look_dir(const GLVec3& eye, const GLVec3& dir, const GLVec3& up);

	void look_info(GLVec3& eye, GLVec3& dir, GLVec3& up);

	/**
	 * @brief forward of dist d in direction of camera
	 * @param alpha angle in degree
	 */
//	void forward(double d);

	inline void set_nav_speed(double s)
	{
		nav_speed_ = s;
	}

	inline void set_nav_speed(const Eigen::Vector3f& dir, double s)
	{
		nav_speed_ = s;
		//*dir.cast<double>();
	}

	inline void set_nav_speed(const Eigen::Vector3d& dir, double s)
	{
		nav_speed_ = s;
		//*dir;
	}

	inline void nav_forward(double dt)
	{
		frame_ = frame_.pretranslate(Eigen::Vector3d(0,0,nav_speed_*dt));
	}



	/**
	 * @brief rotation around Y avis
	 * @param alpha angle in degree
	 */
	void yturn(double alpha);

	/**
	* @brief rotation around X avis
	* @param alpha angle in degree
	*/
	void xturn(double alpha);
};

} // namespace EZCOGL

#endif
