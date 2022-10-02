#pragma once
#include <memory>
#include <string>
#include <texture2d.h>

class TextureImage : public EZCOGL::Texture2D
{
public:
	virtual ~TextureImage();

	static std::shared_ptr<TextureImage> create(const std::string& name);

	const std::string name;
private:
	TextureImage(std::string name);
};
