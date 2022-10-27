#include "post_process_pass.h"

#include <fbo.h>
#include <vao.h>

#include "easycppogl_texture.h"
#include "material.h"
#include "engine/renderer.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"

#include <imgui.h>

static std::unordered_map<std::string, std::shared_ptr<Material>> post_process_materials;

PostProcessPass::~PostProcessPass() {
    parent_renderer.on_resolution_changed.clear_object(this);
}

void PostProcessPass::init(const std::string& fragment_shader) {
    STAT_ACTION("Init post process pass [" + name + "]");
    if (post_process_materials.contains(fragment_shader))
        pass_material = post_process_materials.find(fragment_shader)->second;
    else {
        pass_material = Material::create(name + "::Material");
        pass_material->load_from_source("resources/shaders/post_process_vertex.vs", fragment_shader);
        post_process_materials.insert({fragment_shader, pass_material});
    }
    texture = EasyCppOglTexture::create(name + "::Texture", {
                                            .wrapping = TextureWrapping::ClampToEdge,
                                            .filtering_mag = TextureMagFilter::Linear,
                                            .filtering_min = TextureMinFilter::Linear
                                        });
    texture->set_data_interface(1, 1, GL_RGB16F);
    framebuffer = EZCOGL::FBO::create({texture});
}

void PostProcessPass::bind(bool to_back_buffer) const {
    GL_CHECK_ERROR();
    if (to_back_buffer)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    else
        framebuffer->bind();
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glFrontFace(GL_CCW);
    glClearColor(0, 0, 0, 0);
    glClearDepth(0.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    EZCOGL::VAO::none()->bind();
    GL_CHECK_ERROR();

    pass_material->bind();
    if (previous_texture) {
        glUniform2f(material()->binding("InputResolution"), static_cast<float>(previous_texture->width_interface()),
                    static_cast<float>(previous_texture->height_interface()));
        material()->bind_texture_ex(previous_texture, "InputTexture");
    }
    GL_CHECK_ERROR();
}

void PostProcessPass::draw() const {
    STAT_FRAME("Post processing pass [" + name + "]");
    GL_CHECK_ERROR();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    GL_CHECK_ERROR();
}

int PostProcessPass::width() const {
    return texture->width_interface();
}

int PostProcessPass::height() const {
    return texture->height_interface();
}

PostProcessPass::PostProcessPass(std::string in_name, Renderer& in_parent)
    : RenderPass(std::move(in_name), 1, 1), parent_renderer(in_parent) {
    parent_renderer.on_resolution_changed.add_object(this, &PostProcessPass::resolution_changed);
}

PostProcessPass::PostProcessPass(std::string in_name, const std::shared_ptr<PostProcessPass>& in_previous_pass)
    : PostProcessPass(in_name, in_previous_pass->parent_renderer) {
    previous_texture = in_previous_pass->result();
}

PostProcessPass::PostProcessPass(std::string                                      in_name, Renderer& in_parent,
                                 const std::shared_ptr<EZCOGL::TextureInterface>& in_previous_texture)
    : PostProcessPass(in_name, in_parent) {
    previous_texture = in_previous_texture;
}

void PostProcessPass::resolution_changed(int x, int y) {
    STAT_ACTION("Resize post process pass [" + name + "]");
    if (resolution_changed_callback)
        resolution_changed_callback(x, y);
    if (x < 1)
        x = 1;
    if (y < 1)
        y = 1;
    framebuffer->resize(x, y);
}


/**
 *
 *
 *
 */


void PostProcessPassV2::render(bool to_back_buffer) {
    STAT_FRAME("Post processing pass [" + name + "]");
    RenderPass::render(to_back_buffer);
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glFrontFace(GL_CCW);
    glClearColor(1, 0, 1, 1);
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    EZCOGL::VAO::none()->bind();

    pass_material->bind();

    for (const auto& dep : dependencies) {

        const std::string dep_name = dependencies.size() == 1 ? "" : dep->name + "_";

        ImGui::Text("depname = %s", dep_name.c_str());

        for (const auto& attachment : dep->get_color_attachments()) {
            const std::string att_name    = dep->get_color_attachments().size() == 1 ? "Color" : attachment->name;
            const std::string final_name  = "Input_" + dep_name + att_name;
            const int         res_binding = material()->binding(final_name + "_Res");
            if (res_binding >= 0)
                glUniform2i(res_binding, dep->get_width(), dep->get_height());
            if (attachment->get_render_target())
                material()->bind_texture(attachment->get_render_target(), final_name);
        }
        if (dep->get_depth_attachment()) {
            const std::string final_name  = "Input_Depth";
            const int         res_binding = material()->binding(final_name + "_Res");
            if (res_binding >= 0)
                glUniform2i(res_binding, dep->get_width(), dep->get_height());
            if (dep->get_depth_attachment()->get_render_target())
                material()->bind_texture(dep->get_depth_attachment()->get_render_target(), final_name);
        }
    }

    GL_CHECK_ERROR();
    on_draw.execute();

    GL_CHECK_ERROR();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    GL_CHECK_ERROR();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

PostProcessPassV2::PostProcessPassV2(std::string in_name, uint32_t width, uint32_t height, const std::string& fragment_shader)
    : RenderPass(in_name, width, height) {

    add_attachment("", ImageFormat::RGB_F16, {});

    if (post_process_materials.contains(fragment_shader))
        pass_material = post_process_materials.find(fragment_shader)->second;
    else {
        pass_material = Material::create(name + "::Material");
        pass_material->load_from_source("resources/shaders/post_process_vertex.vs", fragment_shader);
        post_process_materials.insert({fragment_shader, pass_material});
    }
}
