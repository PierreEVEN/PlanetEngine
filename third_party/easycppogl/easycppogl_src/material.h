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


#ifndef EASY_CPP_OGL_MATERIAL_H_
#define EASY_CPP_OGL_MATERIAL_H_

#include <memory>
#include <vector>
#include <string>
#include <gl_eigen.h>
#include <texture2d.h>

namespace EZCOGL
{


class Material
{
public:
	using SP = std::shared_ptr<Material>;
	GLVec3 Ka;
	GLVec3 Kd;
	GLVec3 Ks;
	float Ns;
	float opacity;
	Texture2D::SP tex_ka;
	Texture2D::SP tex_kd;
	Texture2D::SP tex_ks;
	Texture2D::SP tex_ns;
	//	Texture2D::SP tex_rough;
	Texture2D::SP tex_opa; //1
	Texture2D::SP tex_norm_map; //3
	Texture2D::SP tex_reflect; //1
	std::string name_ = "unknown";

	Material() :
		Ka(0.1f, 0.1f, 0.1f), Kd(0.9f, 0.9f, 0.9f), Ks(1.0f, 1.0f, 1.0f), Ns(250), opacity(1.0f), tex_ka(nullptr),
		  tex_kd(nullptr), tex_ks(nullptr), tex_ns(nullptr),
		//tex_rough(nullptr),
		tex_opa(nullptr), tex_norm_map(nullptr), tex_reflect(nullptr)
	{}
	inline void set_name(const std::string& mat_name) 
	{
		if (mat_name.empty())
			name_ = "unknown";
		else
			name_= mat_name;
	}

	inline const std::string& name() const
	{
		return name_;
	}

	bool has_kd_texture() const { return tex_kd != nullptr; }
	bool has_ka_texture() const { return tex_ka != nullptr; }
	bool has_ks_texture() const { return tex_ks != nullptr; }
	bool has_ns_texture() const { return tex_ns != nullptr; }
	bool has_opa_texture() const { return tex_opa != nullptr; }
	bool has_norm_texture() const { return tex_norm_map != nullptr; }
//	bool has_bump_texture() const { return tex_bump_map != nullptr; }
};

}
#endif
