#include "info_ui.h"

#include <imgui.h>
#include <GL/gl3w.h>

#include <utils/gl_tools.h>

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX    0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX  0x9049
#define GL_VBO_FREE_MEMORY_ATI                     0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI                 0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI            0x87FD

InfoUi::InfoUi() {
    window_name = "system information";
    memset(filter_buffer, 0, 256);
}

void InfoUi::draw() {
    ImGui::Text("VENDOR : %s", glGetString(GL_VENDOR));
    ImGui::Text("RENDERER : %s", glGetString(GL_RENDERER));
    ImGui::Text("GL VERSION : %s", glGetString(GL_VERSION));
    ImGui::Text("GLSL VERSION : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    ImGui::Separator();

    GL_CHECK_ERROR();
    int total_gpu_memory = 0;
    glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &total_gpu_memory);
    while (glGetError() != GL_NO_ERROR);
    GL_CHECK_ERROR();
    if (total_gpu_memory != 0) {
        int available_gpu_memory = 0;
        glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &available_gpu_memory);
        ImGui::Text("memory usage : %d%% (%dMo / %dMo)", static_cast<int>(100 - available_gpu_memory / static_cast<float>(total_gpu_memory) * 100),
                    static_cast<int>((total_gpu_memory - available_gpu_memory) / 1000.0), static_cast<int>(total_gpu_memory / 1000.0));
    } else
        ImGui::Text("memory information unavailable");
    GL_CHECK_ERROR();

    ImGui::Separator();

    ImGui::Text("Extensions");
    ImGui::InputText("search", filter_buffer, 256);

    ImGui::Indent();
    int num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

    if (strlen(filter_buffer) == 0) {
        for (int i = 0; i < num_extensions; ++i)
            ImGui::Text("%s", glGetStringi(GL_EXTENSIONS, i));
    } else
        for (int i = 0; i < num_extensions; ++i) {
            std::string extension_name = (char*)glGetStringi(GL_EXTENSIONS, i);
            if (extension_name.find(filter_buffer) != std::string::npos)
                ImGui::Text("%s", extension_name.c_str());
        }
    ImGui::Unindent();

}
