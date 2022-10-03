#pragma once
#include "ui.h"

class TextureManagerUi : public ImGuiWindow
{
public:
	TextureManagerUi() { window_name = "Texture manager"; }
	void draw() override;
};

class MaterialManagerUi : public ImGuiWindow
{
public:
	MaterialManagerUi() { window_name = "Material manager"; }
	void draw() override;
};

class MeshManagerUi : public ImGuiWindow
{
public:
	MeshManagerUi() { window_name = "Mesh manager"; }
	void draw() override;
};

