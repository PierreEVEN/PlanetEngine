#include "texture_image.h"

#include "engine/asset_manager.h"
#include "engine/engine.h"
#include <algorithm>

TextureImage::~TextureImage()
{
	auto& textures = Engine::get().get_asset_manager().textures;
	textures.erase(std::find(textures.begin(), textures.end(), this));
}

std::shared_ptr<TextureImage> TextureImage::create(const std::string& name, const std::vector<GLenum>& params)
{
	return std::shared_ptr<TextureImage>(new TextureImage(name, params));
}
TextureImage::TextureImage(std::string in_name, const std::vector<GLenum>& params) : Texture2D(params), name(std::move(in_name))
{
	Engine::get().get_asset_manager().textures.emplace_back(this);
}
