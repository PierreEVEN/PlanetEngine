#pragma once
#include <vector>

#include "ui.h"

struct Record;

class SessionFrontend final : public ImGuiWindow
{
public:
	SessionFrontend();
	~SessionFrontend() override;

	void draw() override;
private:
	std::vector<Record> last_frame;
};
