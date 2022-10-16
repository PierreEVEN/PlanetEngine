#include "texture_cube.h"

#include <imgui.h>
#include <texture2d.h>

#include "utils/gl_tools.h"

TextureCube::~TextureCube()
{
	if (async_load_thread.joinable())
		async_load_thread.join();
	if (top)
		delete static_cast<EZCOGL::GLImage*>(top);
	if (bottom)
		delete static_cast<EZCOGL::GLImage*>(bottom);
	if (right)
		delete static_cast<EZCOGL::GLImage*>(right);
	if (left)
		delete static_cast<EZCOGL::GLImage*>(left);
	if (front)
		delete static_cast<EZCOGL::GLImage*>(front);
	if (back)
		delete static_cast<EZCOGL::GLImage*>(back);
}

uint32_t TextureCube::texture_type() const
{
	return GL_TEXTURE_CUBE_MAP;
}

void TextureCube::bind(uint32_t unit)
{
	if (id() != 0)
	{
		GL_CHECK_ERROR();
		glBindTexture(GL_TEXTURE_CUBE_MAP, id());
		while (glGetError()); // Skip error messages
	}
	GL_CHECK_ERROR();
}

void TextureCube::from_file(const std::string& file_top, const std::string& file_bottom, const std::string& file_right,
                            const std::string& file_left, const std::string& file_front, const std::string& file_back,
                            int force_nb_channel)
{
	if (async_load_thread.joinable())
		async_load_thread.join();
	async_load_thread = std::thread(
		[&, file_top, file_bottom, file_left, file_right, file_front, file_back, force_nb_channel]
		{
			finished_loading = false;
			std::lock_guard lock_guard(load_mutex);
			top = new EZCOGL::GLImage(file_top, EZCOGL::Texture::flip_y_on_load, force_nb_channel);
			if (!static_cast<EZCOGL::GLImage*>(top)->data())
				std::cerr << "failed to load " << file_top.c_str() << std::endl;
			bottom = new EZCOGL::GLImage(file_bottom, EZCOGL::Texture::flip_y_on_load, force_nb_channel);
			if (!static_cast<EZCOGL::GLImage*>(bottom)->data())
				std::cerr << "failed to load " << file_bottom.c_str() << std::endl;
			right = new EZCOGL::GLImage(file_right, EZCOGL::Texture::flip_y_on_load, force_nb_channel);
			if (!static_cast<EZCOGL::GLImage*>(right)->data())
				std::cerr << "failed to load " << file_right.c_str() << std::endl;
			left = new EZCOGL::GLImage(file_left, EZCOGL::Texture::flip_y_on_load, force_nb_channel);
			if (!static_cast<EZCOGL::GLImage*>(left)->data())
				std::cerr << "failed to load " << file_left.c_str() << std::endl;
			front = new EZCOGL::GLImage(file_front, EZCOGL::Texture::flip_y_on_load, force_nb_channel);
			if (!static_cast<EZCOGL::GLImage*>(front)->data())
				std::cerr << "failed to load " << file_front.c_str() << std::endl;
			back = new EZCOGL::GLImage(file_back, EZCOGL::Texture::flip_y_on_load, force_nb_channel);
			if (!static_cast<EZCOGL::GLImage*>(back)->data())
				std::cerr << "failed to load " << file_back.c_str() << std::endl;
			finished_loading = true;
		});
}

void TextureCube::set_data(int32_t w, int32_t h, uint32_t in_image_format, const void* data_top,
                           const void* data_bottom,
                           const void* data_left, const void* data_right, const void* data_front, const void* data_back)
{
	image_format = in_image_format;
	const auto tf = EZCOGL::Texture::texture_formats[image_format];
	external_format = tf.first;
	image_width = w;
	image_height = h;
	data_format = tf.second;
	bind();
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, image_format, w, h, 0, external_format, data_format,
	             (w * h > 0) ? data_right : nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, image_format, w, h, 0, external_format, data_format,
	             (w * h > 0) ? data_left : nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, image_format, w, h, 0, external_format, data_format,
	             (w * h > 0) ? data_top : nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, image_format, w, h, 0, external_format, data_format,
	             (w * h > 0) ? data_bottom : nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, image_format, w, h, 0, external_format, data_format,
	             (w * h > 0) ? data_front : nullptr);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, image_format, w, h, 0, external_format, data_format,
	             (w * h > 0) ? data_back : nullptr);
	glBindTexture(texture_type(), 0);
	GL_CHECK_ERROR();
}

uint32_t TextureCube::id()
{
	if (finished_loading)
	{
		std::lock_guard lock_guard(load_mutex);
		finished_loading = false;
		const auto* top_image = static_cast<EZCOGL::GLImage*>(top);
		switch (top_image->depth())
		{
		case 1:
			set_data(static_cast<EZCOGL::GLImage*>(top)->width(), static_cast<EZCOGL::GLImage*>(top)->height(), GL_R8,
			         static_cast<EZCOGL::GLImage*>(top)->data(),
			         static_cast<EZCOGL::GLImage*>(bottom)->data(),
			         static_cast<EZCOGL::GLImage*>(left)->data(),
			         static_cast<EZCOGL::GLImage*>(right)->data(),
			         static_cast<EZCOGL::GLImage*>(front)->data(),
			         static_cast<EZCOGL::GLImage*>(back)->data());
			break;
		case 3:
			set_data(static_cast<EZCOGL::GLImage*>(top)->width(), static_cast<EZCOGL::GLImage*>(top)->height(), GL_RGB8,
			         static_cast<EZCOGL::GLImage*>(top)->data(),
			         static_cast<EZCOGL::GLImage*>(bottom)->data(),
			         static_cast<EZCOGL::GLImage*>(left)->data(),
			         static_cast<EZCOGL::GLImage*>(right)->data(),
			         static_cast<EZCOGL::GLImage*>(front)->data(),
			         static_cast<EZCOGL::GLImage*>(back)->data());
			break;
		case 4:
			set_data(static_cast<EZCOGL::GLImage*>(top)->width(), static_cast<EZCOGL::GLImage*>(top)->height(),
			         GL_RGBA8,
			         static_cast<EZCOGL::GLImage*>(top)->data(),
			         static_cast<EZCOGL::GLImage*>(bottom)->data(),
			         static_cast<EZCOGL::GLImage*>(left)->data(),
			         static_cast<EZCOGL::GLImage*>(right)->data(),
			         static_cast<EZCOGL::GLImage*>(front)->data(),
			         static_cast<EZCOGL::GLImage*>(back)->data());
			break;
		default:
			delete static_cast<EZCOGL::GLImage*>(top);
			delete static_cast<EZCOGL::GLImage*>(bottom);
			delete static_cast<EZCOGL::GLImage*>(left);
			delete static_cast<EZCOGL::GLImage*>(right);
			delete static_cast<EZCOGL::GLImage*>(front);
			delete static_cast<EZCOGL::GLImage*>(back);

			top = nullptr;
			bottom = nullptr;
			right = nullptr;
			left = nullptr;
			front = nullptr;
			back = nullptr;
			return 0;
		}

		bind();
		glGenerateMipmap(texture_type());
		glBindTexture(texture_type(), 0);

		delete static_cast<EZCOGL::GLImage*>(top);
		delete static_cast<EZCOGL::GLImage*>(bottom);
		delete static_cast<EZCOGL::GLImage*>(left);
		delete static_cast<EZCOGL::GLImage*>(right);
		delete static_cast<EZCOGL::GLImage*>(front);
		delete static_cast<EZCOGL::GLImage*>(back);

		top = nullptr;
		bottom = nullptr;
		right = nullptr;
		left = nullptr;
		front = nullptr;
		back = nullptr;
		GL_CHECK_ERROR();
	}


	return TextureBase::id();
}

static TextureCreateInfos cubemap_params(TextureCreateInfos params)
{
	params.wrapping = TextureWrapping::ClampToEdge;
	return params;
}

TextureCube::TextureCube(std::string name, const TextureCreateInfos& params)
	: TextureBase(name, cubemap_params(params))
{
}
