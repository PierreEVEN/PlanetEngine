

#include "easycppogl_texture.h"

EasyCppOglTexture::~EasyCppOglTexture()
{
}

EasyCppOglTexture::EasyCppOglTexture(std::string name, const TextureCreateInfos& params)
	: Texture2D(name, params)
{
}