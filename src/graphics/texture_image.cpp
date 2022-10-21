#include "texture_image.h"

#include "engine/asset_manager.h"
#include "engine/engine.h"
#include <algorithm>
#include <functional>
#include <texture2d.h>
#include <GL/gl3w.h>

#include "utils/gl_tools.h"
#include "utils/profiler.h"

TextureBase::~TextureBase()
{
	glDeleteTextures(1, &texture_id);
}

std::shared_ptr<TextureBase> TextureBase::create(const std::string& name, const TextureCreateInfos& params)
{
	return std::shared_ptr<TextureBase>(new TextureBase(name, params));
}

void TextureBase::bind(uint32_t unit)
{
	if (id() != 0) {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(texture_type(), id());
		GL_CHECK_ERROR();
	}
}

bool TextureBase::is_depth() const
{
	return image_format == GL_DEPTH_COMPONENT32F || image_format == GL_DEPTH_COMPONENT24;
}

uint32_t TextureBase::texture_type() const
{
	return GL_TEXTURE_2D;
}

TextureBase::TextureBase(std::string in_name, const TextureCreateInfos& params) : name(std::move(in_name))
{
	GL_CHECK_ERROR();
	glGenTextures(1, &texture_id);
	bind();

	int min_filter = -1;
	switch (params.filtering_min)
	{
	case TextureMinFilter::Nearest: min_filter = GL_NEAREST;
		break;
	case TextureMinFilter::Linear: min_filter = GL_LINEAR;
		break;
	case TextureMinFilter::MipMap_NearestNearest: min_filter = GL_NEAREST_MIPMAP_NEAREST;
		break;
	case TextureMinFilter::MipMap_LinearNearest: min_filter = GL_LINEAR_MIPMAP_NEAREST;
		break;
	case TextureMinFilter::MipMap_NearestLinear: min_filter = GL_NEAREST_MIPMAP_LINEAR;
		break;
	case TextureMinFilter::MipMap_LinearLinear: min_filter = GL_LINEAR_MIPMAP_LINEAR;
		break;
	}

	int mag_filter = -1;
	switch (params.filtering_mag)
	{
	case TextureMagFilter::Nearest: mag_filter = GL_NEAREST;
		break;
	case TextureMagFilter::Linear: mag_filter = GL_LINEAR;
		break;
	}

	int wrapping = -1;
	switch (params.wrapping)
	{
	case TextureWrapping::Repeat: wrapping = GL_REPEAT;
		break;
	case TextureWrapping::MirroredRepeat: wrapping = GL_MIRRORED_REPEAT;
		break;
	case TextureWrapping::ClampToEdge: wrapping = GL_CLAMP_TO_EDGE;
		break;
	case TextureWrapping::ClampToBorder: wrapping = GL_CLAMP_TO_BORDER;
		break;
	}

	glTexParameteri(texture_type(), GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(texture_type(), GL_TEXTURE_MAG_FILTER, mag_filter);

	glTexParameteri(texture_type(), GL_TEXTURE_WRAP_S, wrapping);
	glTexParameteri(texture_type(), GL_TEXTURE_WRAP_T, wrapping);
	glTexParameteri(texture_type(), GL_TEXTURE_WRAP_R, wrapping);
	GL_CHECK_ERROR();
}

Texture2D::~Texture2D()
{
	auto& textures = Engine::get().get_asset_manager().textures;
	textures.erase(std::find(textures.begin(), textures.end(), this));
	if (async_load_thread.joinable())
		async_load_thread.join();

	if (loading_image_ptr)
		delete static_cast<EZCOGL::GLImage*>(loading_image_ptr);
}

void Texture2D::from_file(const std::string& filename, int force_nb_channel)
{
	if (async_load_thread.joinable())
		async_load_thread.join();

	async_load_thread = std::thread([&, filename, force_nb_channel]
	{
		std::lock_guard lock_guard(load_mutex);
		STAT_ACTION("set texture data [" + filename + "]");
		finished_loading = false;
		loading_image_ptr = new EZCOGL::GLImage(filename, EZCOGL::Texture::flip_y_on_load, force_nb_channel);
		finished_loading = true;
	});
}

void Texture2D::set_data(int32_t w, int32_t h, uint32_t in_image_format, const void* data_ptr)
{
	image_format = in_image_format;
	auto tf = EZCOGL::Texture::texture_formats[image_format];
	external_format = tf.first;
	image_width = w;
	image_height = h;
	data_format = tf.second;
	GL_CHECK_ERROR();
	bind();
	GL_CHECK_ERROR();
	glTexImage2D(texture_type(), 0, data_ptr ? GL_SRGB : image_format, w, h, 0, external_format, data_format,
	             (w * h > 0) ? data_ptr : nullptr);
	GL_CHECK_ERROR();
	glBindTexture(texture_type(), 0);

	GL_CHECK_ERROR();
}

uint32_t Texture2D::id()
{
	if (finished_loading)
	{
		STAT_ACTION("set texture data [" + name + "]");
		std::lock_guard lock_guard(load_mutex);
		finished_loading = false;

		const auto* image = static_cast<EZCOGL::GLImage*>(loading_image_ptr);

		switch (image->depth())
		{
		case 1:
			set_data(image->width(), image->height(), GL_R8, image->data());
			break;
		case 3:
			set_data(image->width(), image->height(), GL_RGB8, image->data());
			break;
		case 4:
			set_data(image->width(), image->height(), GL_RGBA8, image->data());
			break;
		default:
			delete image;
			loading_image_ptr = nullptr;
			return 0;
		}

		bind();
		glGenerateMipmap(texture_type());
		glBindTexture(texture_type(), 0);

		GL_CHECK_ERROR();
		delete image;
		loading_image_ptr = nullptr;
	}


	return TextureBase::id();
}

Texture2D::Texture2D(std::string name, const TextureCreateInfos& params)
	: TextureBase(name, params)
{
	Engine::get().get_asset_manager().textures.emplace_back(this);

}
