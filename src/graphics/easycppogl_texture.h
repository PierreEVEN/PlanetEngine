#pragma once

#include <texture_interface.h>

#include "texture_image.h"

class EasyCppOglTexture : public Texture2D, public EZCOGL::TextureInterface
{
public:
	~EasyCppOglTexture() override;

	static std::shared_ptr<EasyCppOglTexture> create(const std::string& name, const TextureCreateInfos& params = {})
	{
		return std::shared_ptr<EasyCppOglTexture>(new EasyCppOglTexture(name, params));
	}

private:
	EasyCppOglTexture(std::string name, const TextureCreateInfos& params = {});

public:
	void init_interface(GLint internal) override
	{
		return set_data(0, 0, image_format, nullptr);
	}
	int id_interface() override
	{
		return id();
	}
	void resize_interface(GLsizei w, GLsizei h) override
	{
		return set_data(w, h, internal_format());
	}
	GLsizei width_interface() const override
	{
		return width();
	}
	GLsizei height_interface() const override
	{
		return height();
	}
	void set_data_interface(int32_t w, int32_t h, uint32_t image_format, const void* data_ptr = nullptr) override
	{
		return set_data(w, h, static_cast<ImageFormat>(image_format), data_ptr);
	}
};