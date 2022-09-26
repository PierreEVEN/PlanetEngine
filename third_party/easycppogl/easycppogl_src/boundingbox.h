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


#ifndef EASY_CPP_OGL_BB_H_
#define EASY_CPP_OGL_BB_H_

#include <gl_eigen.h>
#include <memory>
#include <tuple>
#include <vector>


namespace EZCOGL
{

class BoundingBox
{
protected:
	GLVec3 min_;
	GLVec3 max_;
	bool initialized_;
	void direct_add_point(const GLVec3& P);

	inline BoundingBox() : initialized_(false)
	{
	}

public:
	using SP = std::shared_ptr<BoundingBox>;

	inline bool is_initialized() const
	{
		return initialized_;
	}

	inline static BoundingBox::SP create()
	{
		return std::shared_ptr<BoundingBox>(new BoundingBox{});
	}

	void add_point(const GLVec3& P);

	inline GLVec3 center() const
	{
		return (min_ + max_) / 2.0;
	}

	inline float radius() const
	{
		GLVec3 dv = max_ - min_;
		return dv.norm() / 2.0f;
	}

	inline GLMat4 matrix() const
	{
		return Transfo::translate(center()) * Transfo::scale((max_ - min_) / 2.0f);
	}

	inline const GLVec3& min() const
	{
		return min_;
	}
	inline const GLVec3& max() const
	{
		return max_;
	}

	void merge(const BoundingBox& bb);

	void transform(const GLMat4& trf);

	BoundingBox::SP transformed(const GLMat4& trf);
};


}
#endif
