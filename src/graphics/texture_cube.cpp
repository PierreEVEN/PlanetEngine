#include "texture_cube.h"

#include <imgui.h>
#include <texture2d.h>

#include "utils/gl_tools.h"
#include "utils/profiler.h"

TextureCube::~TextureCube()
{
	for (auto& thread : async_load_thread)
		if (thread.joinable())
			thread.join();
	for (const auto& ptr : loaded_image_ptr)
		if (ptr)
			delete static_cast<EZCOGL::GLImage*>(ptr);
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
	for (auto& thread : async_load_thread)
		if (thread.joinable())
			thread.join();

	std::lock_guard lock_guard(load_mutex);
	const auto files = std::vector{file_right, file_left, file_top, file_bottom, file_front, file_back};
	for (size_t i = 0; i < files.size(); ++i)
		async_load_thread[i] = std::thread(
			[&, files, i, force_nb_channel]
			{
				STAT_ACTION("load cubemap [" + files[i] + "]::" + std::to_string(i));
				finished_loading[i] = false;
				loaded_image_ptr[i] = new EZCOGL::GLImage(files[i], EZCOGL::Texture::flip_y_on_load, force_nb_channel);
				if (!static_cast<EZCOGL::GLImage*>(loaded_image_ptr[i])->data())
					std::cerr << "failed to load " << file_top.c_str() << std::endl;
				finished_loading[i] = true;
			});
}

void TextureCube::set_data(int32_t w, int32_t h, uint32_t in_image_format, uint32_t index, const void* image_data)
{
	STAT_ACTION("set cubemap data [" + name + "]::" + std::to_string(index));
	image_format = in_image_format;
	const auto tf = EZCOGL::Texture::texture_formats[image_format];
	external_format = tf.first;
	image_width = w;
	image_height = h;
	data_format = tf.second;
	bind();
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, GL_SRGB, w, h, 0, external_format, data_format,
	             (w * h > 0) ? image_data : nullptr);
	glBindTexture(texture_type(), 0);
	GL_CHECK_ERROR();
}

uint32_t TextureCube::id()
{
	if (std::find(finished_loading.begin(), finished_loading.end(), false) == finished_loading.end())
	{
		std::lock_guard lock_guard(load_mutex);
		finished_loading.fill(false);

		for (size_t i = 0; i < finished_loading.size(); ++i)
		{
			const auto* image = static_cast<EZCOGL::GLImage*>(loaded_image_ptr[i]);
			switch (image->depth())
			{
			case 1:
				set_data(image->width(), image->height(), GL_R8, static_cast<uint32_t>(i), image->data());
				break;
			case 3:
				set_data(image->width(), image->height(), GL_RGB8, static_cast<uint32_t>(i), image->data());
				break;
			case 4:
				set_data(image->width(), image->height(), GL_RGBA8, static_cast<uint32_t>(i), image->data());
				break;
			default:
				break;
			}
			delete image;
			loaded_image_ptr[i] = nullptr;
		}

		bind();
		glGenerateMipmap(texture_type());
		glBindTexture(texture_type(), 0);
		GL_CHECK_ERROR();
	}


	return
		TextureBase::id();
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
