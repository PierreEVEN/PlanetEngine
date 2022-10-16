#include "post_process_pass.h"

#include <fbo.h>
#include <vao.h>

#include "easycppogl_texture.h"
#include "material.h"
#include "engine/renderer.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"

PostProcessPass::~PostProcessPass()
{
	parent_renderer.on_resolution_changed.clear_object(this);
}

static std::unordered_map<std::string, std::shared_ptr<Material>> post_process_materials;

void PostProcessPass::init(const std::string& fragment_shader)
{
	if (post_process_materials.contains(fragment_shader))
		pass_material = post_process_materials.find(fragment_shader)->second;
	else {
		pass_material = Material::create(name + "::Material");
		pass_material->load_from_source("resources/shaders/post_process_vertex.vs", fragment_shader);
		post_process_materials.insert({ fragment_shader, pass_material });
	}
	texture = EasyCppOglTexture::create(name + "::Texture", { .wrapping = TextureWrapping::ClampToEdge, .filtering_mag = TextureMagFilter::Linear, .filtering_min = TextureMinFilter::Linear });
	texture->set_data_interface(1920, 1080, GL_RGB16F);
	framebuffer = EZCOGL::FBO::create({texture});
}

void PostProcessPass::bind(bool to_back_buffer) const
{
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
	GL_CHECK_ERROR();
}

void PostProcessPass::draw() const
{
	STAT_DURATION("Post processing pass [" + name + "]");
	GL_CHECK_ERROR();
	glDrawArrays(GL_TRIANGLES, 0, 3);
	GL_CHECK_ERROR();
}

int PostProcessPass::width() const
{
	return texture->width_interface();
}

int PostProcessPass::height() const
{
	return texture->height_interface();
}

PostProcessPass::PostProcessPass(std::string in_name, Renderer& in_parent)
	: name(std::move(in_name)),
	  parent_renderer(in_parent)
{
	parent_renderer.on_resolution_changed.add_object(this, &PostProcessPass::resolution_changed);
}

void PostProcessPass::resolution_changed(int x, int y)
{
	if (resolution_changed_callback)
		resolution_changed_callback(x, y);
	if (x < 1) x = 1;
	if (y < 1) y = 1;
	framebuffer->resize(x, y);
}
