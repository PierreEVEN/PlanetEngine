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

#ifndef EASY_CPP_OGL_TEXTURE_H_
#define EASY_CPP_OGL_TEXTURE_H_

#include <GL/gl3w.h>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include "texture_interface.h"

namespace EZCOGL
{

	class Texture : public TextureInterface
	{
	protected:
		GLuint id_;
		GLint internal_;
		GLenum external_;
		GLenum data_type_;
		GLsizei width_;
		bool depth_;

	protected:
		inline Texture() :
			id_(0),
			internal_(0),
			external_(0),
			data_type_(0),
			width_(0),
			depth_(false)
		{
		}

	public:
		static bool flip_y_on_load;
		static std::map<GLint, std::pair<GLenum, GLenum>> texture_formats;

		inline virtual GLuint id()
		{
			return id_;
		}

		int id_interface() override { return id(); }

		inline virtual GLsizei width() const
		{
			return width_;
		}
		GLsizei width_interface() const override
		{
			return width();
		}

		inline GLuint bind_compute(GLuint img_binding, GLenum in_out)
		{
			glBindImageTexture(img_binding, id_, 0, GL_FALSE, 0, in_out, internal_);
			return img_binding;
		}

		inline static void unbind_compute_in(GLuint img_binding)
		{
			glBindImageTexture(img_binding, 0, 0, GL_FALSE, 0, GL_READ_ONLY, 0);
		}

		inline static void unbind_compute_out(GLuint img_binding)
		{
			glBindImageTexture(img_binding, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, 0);
		}
	};
}

#endif
