#include "info_ui.h"

#include <imgui.h>
#include <GL/gl3w.h>

void InfoUi::draw()
{
	ImGui::Text("VENDOR : %s", glGetString(GL_VENDOR));
	ImGui::Text("RENDERER : %s", glGetString(GL_RENDERER));
	ImGui::Text("GL VERSION : %s", glGetString(GL_VERSION));
}
