#include "info_ui.h"

#include <imgui.h>
#include <GL/gl3w.h>

#include <utils/gl_tools.h>

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX    0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX  0x9049
#define GL_VBO_FREE_MEMORY_ATI                     0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI                 0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI            0x87FD

void InfoUi::draw()
{
	ImGui::Text("VENDOR : %s", glGetString(GL_VENDOR));
	ImGui::Text("RENDERER : %s", glGetString(GL_RENDERER));
	ImGui::Text("GL VERSION : %s", glGetString(GL_VERSION));
	ImGui::Text("GLSL VERSION : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	ImGui::Separator();
	ImGui::Text("Extensions");
	ImGui::Indent();
	int num_extensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
	for (int i = 0; i < num_extensions; ++i)
		ImGui::Text("%s", glGetStringi(GL_EXTENSIONS, i));
	ImGui::Unindent();
	ImGui::Separator();

	GL_CHECK_ERROR();
	int total_gpu_memory = 0;
	glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &total_gpu_memory);
	while (glGetError() != GL_NO_ERROR);
	GL_CHECK_ERROR();
	if (total_gpu_memory != 0) {
		int available_gpu_memory = 0;
		glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &available_gpu_memory);
		ImGui::Text("available memory : %f % (%f / %f)", available_gpu_memory / (float)total_gpu_memory, available_gpu_memory, total_gpu_memory);
	}
	else
		ImGui::Text("memory information unavailable");
	GL_CHECK_ERROR();
}
