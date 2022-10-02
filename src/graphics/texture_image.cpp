#include "texture_image.h"

#include "engine/asset_manager.h"
#include "engine/engine.h"

TextureImage::~TextureImage()
{
	auto& textures = Engine::get().get_asset_manager().textures;
	textures.erase(std::ranges::find(textures, this));
}

std::shared_ptr<TextureImage> TextureImage::create(const std::string& name)
{
	return std::shared_ptr<TextureImage>(new TextureImage(name));
}
TextureImage::TextureImage(std::string in_name) : Texture2D(), name(std::move(in_name))
{
	Engine::get().get_asset_manager().textures.emplace_back(this);
}
