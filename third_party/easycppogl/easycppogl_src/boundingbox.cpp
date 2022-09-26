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

#include <boundingbox.h>
#include <iostream>

#pragma warning( disable : 4244 4245 4018)

namespace EZCOGL
{

const float M_PIF = float(M_PI);

void BoundingBox::direct_add_point(const GLVec3& P)
{
    min_[0] = std::min(min_[0],P[0]);
    min_[1] = std::min(min_[1],P[1]);
    min_[2] = std::min(min_[2],P[2]);
    max_[0] = std::max(max_[0],P[0]);
    max_[1] = std::max(max_[1],P[1]);
    max_[2] = std::max(max_[2],P[2]);
}

void BoundingBox::add_point(const GLVec3& P)
{
	if (!initialized_)
	{
		min_ = P;
		max_ = P;
		initialized_ = true;
	}
	else
		direct_add_point(P);
}

void BoundingBox::merge(const BoundingBox& bb)
{
	if (!initialized_)
	{
		min_ = bb.min_;
		max_ = bb.max_;
		if (bb.initialized_)
			initialized_ = true;
		else
			std::cerr << "Warning merging 2 uninitialized Bounding Box" << std::endl;
	}
	else
	{
		min_[0] = std::min(min_[0],bb.min_[0]);
		min_[1] = std::min(min_[1],bb.min_[1]);
		min_[2] = std::min(min_[2],bb.min_[2]);
		max_[0] = std::max(max_[0],bb.max_[0]);
		max_[1] = std::max(max_[1],bb.max_[1]);
		max_[2] = std::max(max_[2],bb.max_[2]);
	}
}

void BoundingBox::transform(const GLMat4& trf)
{
	min_ = Transfo::apply(trf,min_);
	max_ = Transfo::apply(trf,max_);
}

BoundingBox::SP BoundingBox::transformed(const GLMat4& trf)
{
	BoundingBox::SP bb= BoundingBox::create();
	bb->initialized_ = true;
	bb->min_ = Transfo::apply(trf,min_);
	bb->max_ = Transfo::apply(trf, max_);
	return bb;
}

}
