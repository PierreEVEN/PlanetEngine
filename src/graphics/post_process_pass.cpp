#include "post_process_pass.h"

#include <vao.h>

#include "material.h"
#include "engine/renderer.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"

#include <imgui.h>

static std::unordered_map<std::string, std::shared_ptr<Material>> post_process_materials;

void PostProcessPass::render(bool to_back_buffer) {
    if (!pre_render())
        return;
    STAT_FRAME("Post processing pass [" + name + "]");
    GL_CHECK_ERROR();
    bind(to_back_buffer);
    GL_CHECK_ERROR();
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glFrontFace(GL_CCW);
    glClearColor(1, 0, 1, 1);
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL_CHECK_ERROR();
    EZCOGL::VAO::bind_none();
    GL_CHECK_ERROR();
    pass_material->bind();

    for (const auto& dep : dependencies) {
        const auto& bp = bind_point_names(dep);
        const auto& rt = dep->get_all_render_targets();
        for (size_t i = 0; i < rt.size(); ++i) {
            if (const int res_binding = material()->binding(bp[i] + "_Res"); res_binding >= 0)
                glUniform2i(res_binding, dep->get_width(), dep->get_height());
            GL_CHECK_ERROR();
            material()->set_texture(bp[i], rt[i]);
        }

    }
    GL_CHECK_ERROR();
    on_bind_material.execute(pass_material);

    GL_CHECK_ERROR();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    GL_CHECK_ERROR();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

PostProcessPass::PostProcessPass(std::string in_name, uint32_t width, uint32_t height, const std::string& fragment_shader, TextureCreateInfos create_infos)
    : RenderPass(in_name, width, height) {

    add_attachment("", ImageFormat::RGB_F16, create_infos);

    if (post_process_materials.contains(fragment_shader))
        pass_material = post_process_materials.find(fragment_shader)->second;
    else {
        pass_material = Material::create(name + "::Material", "resources/shaders/post_process_vertex.vs", fragment_shader);
        post_process_materials.insert({fragment_shader, pass_material});
    }
}
