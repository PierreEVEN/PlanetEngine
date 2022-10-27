#include "render_pass.h"

#include <string>
#include <utility>
#include <GL/gl3w.h>

#include "texture_image.h"
#include "utils/gl_tools.h"

#include <vao.h>

TextureAttachment::TextureAttachment(std::string framebuffer_name, std::string in_name, const TextureCreateInfos& create_infos, ImageFormat in_format, int in_binding_index,
                                     uint32_t    framebuffer)
    : Attachment(std::move(in_name), in_format, in_binding_index) {
    render_target = Texture2D::create(framebuffer_name + "_" + name, create_infos);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, is_depth_format(format) ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0 + binding_index, GL_TEXTURE_2D, render_target->id(), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TextureAttachment::init(uint32_t width, uint32_t height) {
    render_target->set_data(width, height, format, nullptr);
}

RenderBufferAttachment::RenderBufferAttachment(std::string in_name, ImageFormat in_format, int binding_index, uint32_t framebuffer)
    : Attachment(std::move(in_name), in_format, binding_index), rbo(0) {
    glGenRenderbuffers(1, &rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, is_depth_format(format) ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0 + binding_index, GL_RENDERBUFFER, rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RenderBufferAttachment::~RenderBufferAttachment() {
    glDeleteRenderbuffers(1, &rbo);
}

void RenderBufferAttachment::init(uint32_t width, uint32_t height) {
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLenum>(format), width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

RenderPass::RenderPass(std::string name, uint32_t in_width, uint32_t in_height)
    : name(std::move(name)), is_dirty(true), width(in_width), height(in_height) {
    glGenFramebuffers(1, &framebuffer_id);
}

void RenderPass::init_attachments() {
    if (depth_attachment)
        depth_attachment->init(width, height);
    for (const auto& color_attachment : color_attachments)
        color_attachment->init(width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    std::vector<uint32_t> attachment_ids(color_attachments.size());
    for (int i            = 0; i < color_attachments.size(); ++i)
        attachment_ids[i] = GL_COLOR_ATTACHMENT0 + i;
    glDrawBuffers(static_cast<GLsizei>(attachment_ids.size()), attachment_ids.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    is_dirty = false;
}

RenderPass::~RenderPass() {
    glDeleteFramebuffers(1, &framebuffer_id);
}

void RenderPass::render(bool to_back_buffer) {
    if (width == 0 || height == 0)
        return;

    if (complete)
        return;
    complete = true;

    for (const auto& dep : dependencies)
        dep->render(false);

    if (is_dirty)
        init_attachments();

    GL_CHECK_ERROR();
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    glViewport(0, 0, width, height);
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glFrontFace(GL_CCW);
    glClearColor(0, 0, 0, 0);
    glClearDepth(0.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    EZCOGL::VAO::none()->bind();

    GL_CHECK_ERROR();
    on_draw.execute();
}

void RenderPass::reset() {
    complete = false;
    for (const auto& dep : dependencies)
        dep->reset();
}

void RenderPass::resize(uint32_t in_width, uint32_t in_height) {
    for (const auto& dep : dependencies)
        dep->resize(in_width, in_height);

    if (compute_resolution)
        compute_resolution(in_width, in_height);

    if (width == in_width && height == in_height)
        return;

    width    = in_width;
    height   = in_height;
    is_dirty = true;
}

void RenderPass::add_attachment(const std::string& attachment_name, ImageFormat image_format, const TextureCreateInfos& create_infos, bool write_only) {
    const int         binding_index = static_cast<int>(color_attachments.size());
    if (is_depth_format(image_format))
        if (write_only)
            depth_attachment = std::make_unique<RenderBufferAttachment>(attachment_name, image_format, binding_index, framebuffer_id);
        else
            depth_attachment = std::make_unique<TextureAttachment>(name, attachment_name, create_infos, image_format, binding_index, framebuffer_id);
    else if (write_only)
        color_attachments.emplace_back(std::make_unique<RenderBufferAttachment>(attachment_name, image_format, binding_index, framebuffer_id));
    else
        color_attachments.emplace_back(std::make_unique<TextureAttachment>(name, attachment_name, create_infos, image_format, binding_index, framebuffer_id));

    is_dirty = true;
}
