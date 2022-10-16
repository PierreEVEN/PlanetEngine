

#include "easycppogl_texture.h"

#include "engine/asset_manager.h"
#include "engine/engine.h"

EasyCppOglTexture::~EasyCppOglTexture()
{
	auto& textures = Engine::get().get_asset_manager().textures;
	textures.erase(std::find(textures.begin(), textures.end(), this));
}

EasyCppOglTexture::EasyCppOglTexture(std::string name, const TextureCreateInfos& params)
	: Texture2D(name, params)
{
	Engine::get().get_asset_manager().textures.emplace_back(this);
}