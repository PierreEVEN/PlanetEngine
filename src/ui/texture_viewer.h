#pragma once
#include "ui.h"


class TextureBase;

class TextureViewer : public ImGuiWindow
{
public:
	TextureViewer(TextureBase* texture);
	void draw() override;
private:
	TextureBase* texture;
};
