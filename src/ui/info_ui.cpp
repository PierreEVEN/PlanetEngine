#include "info_ui.h"

#include <imgui.h>
#include <GL/gl3w.h>

inline void InfoUi::draw()
{
	ImGui::Text("%s", glGetString(GL_VENDOR));
	ImGui::Text("%s", glGetString(GL_RENDERER));
	ImGui::Text("%s", glGetString(GL_VERSION));
}
