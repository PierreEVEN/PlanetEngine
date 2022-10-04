#pragma once
#include "ui.h"

class WorldOutliner : public ImGuiWindow
{
public:
	WorldOutliner(const World* in_world) : world(in_world)
	{
		window_name = "World outliner";
	}

	void draw() override;
private:
	const World* world;
};
