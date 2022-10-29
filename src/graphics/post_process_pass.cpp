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
    for (size_t i = 0; i < dependencies.size(); ++i) {

        const auto& dep = dependencies[i];

        const std::string dep_name   = dependencies.size() == 1 ? "" : dep->name + "_";
        const auto&       bind_point = bind_points.find(i);

        for (size_t j = 0; j < dep->get_color_attachments().size(); ++j) {

            const auto& attachment = dep->get_color_attachments()[j];

            const std::string att_name   = dep->get_color_attachments().size() == 1 ? "Color" : attachment->name;
            std::string       final_name = "Input_" + dep_name + att_name;

            if (bind_point != bind_points.end())
                final_name = bind_point->second.first[j];

            const int res_binding = material()->binding(final_name + "_Res");
            GL_CHECK_ERROR();
            if (res_binding >= 0)
                glUniform2i(res_binding, dep->get_width(), dep->get_height());
            GL_CHECK_ERROR();
            if (attachment->get_render_target())
                material()->bind_texture(attachment->get_render_target(), final_name);
            GL_CHECK_ERROR();
        }
        if (dep->get_depth_attachment()) {
            std::string final_name = "Input_Depth";
            if (bind_point != bind_points.end())
                final_name = bind_point->second.second;

            const int res_binding = material()->binding(final_name + "_Res");
            if (res_binding >= 0)
                glUniform2i(res_binding, dep->get_width(), dep->get_height());
            if (dep->get_depth_attachment()->get_render_target())
                material()->bind_texture(dep->get_depth_attachment()->get_render_target(), final_name);
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
        pass_material = Material::create(name + "::Material");
        pass_material->load_from_source("resources/shaders/post_process_vertex.vs", fragment_shader);
        post_process_materials.insert({fragment_shader, pass_material});
    }
}
