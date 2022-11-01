#pragma once
#include "ui.h"

class InfoUi : public ImGuiWindow {
public:
    InfoUi();
    void draw() override;
    char filter_buffer[256];
};
