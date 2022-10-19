#pragma once
#include "ui.h"

class InfoUi : public ImGuiWindow
{
public:
	InfoUi() { window_name = "system information"; }
	void draw() override;
};