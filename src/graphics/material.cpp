#include "material.h"

#include <filesystem>
#include <fstream>
#include <GL/gl3w.h>

#include <shader_program.h>

#include "easycppogl_texture.h"
#include "texture_image.h"
#include "engine/asset_manager.h"
#include "engine/engine.h"
#include "utils/gl_tools.h"
#include "utils/profiler.h"

int Material::binding(const std::string& binding_name) const
{
	const auto binding = bindings.find(binding_name);
	if (binding != bindings.end())
		return binding->second;
	return -1;
}

int Material::bind_texture(const std::shared_ptr<TextureBase>& texture, const std::string& binding_name) const
{
	GL_CHECK_ERROR();
	const int binding_location = binding(binding_name);
	if (binding_location > 0)
	{
		glUniform1i(binding_location, binding_location);
		GL_CHECK_ERROR();
		texture->bind(binding_location);
		GL_CHECK_ERROR();
	}
	GL_CHECK_ERROR();
	return binding_location;
}

int Material::bind_texture_ex(const std::shared_ptr<EZCOGL::TextureInterface>& texture,
	const std::string& binding_name) const
{
	return bind_texture(dynamic_pointer_cast<EasyCppOglTexture>(texture), binding_name);
}

Material::Material(const std::string& in_name) : name(in_name), shader_program_id(0)
{
	Engine::get().get_asset_manager().materials.emplace_back(this);
	vertex_source.on_data_changed.add_object(this, &Material::mark_dirty);
	fragment_source.on_data_changed.add_object(this, &Material::mark_dirty);
}

void Material::reload_internal()
{
	GL_CHECK_ERROR();
	if (shader_program_id != 0)
		glDeleteProgram(shader_program_id);

	GL_CHECK_ERROR();
	compilation_error.reset();
	bindings.clear();
	// Compile shader
	shader_program_id = glCreateProgram();
	program_vertex = std::make_unique<EZCOGL::Shader>(GL_VERTEX_SHADER);
	std::string vertex_error;
	size_t vertex_error_line;
	if (!program_vertex->compile(vertex_source.get_source_code(), name, vertex_error, vertex_error_line))
	{
		std::cout << "error " << vertex_error << std::endl;
		size_t local_line;
		compilation_error = {
			.error = vertex_error,
			.line = 0,
			.file = vertex_source.get_file_at_line(vertex_error_line, local_line),
			.is_fragment = false,
		};
		compilation_error->line = local_line;
		glDeleteProgram(shader_program_id);
		shader_program_id = 0;
		is_dirty = false;
		return;
	}
	GL_CHECK_ERROR();
	glAttachShader(shader_program_id, program_vertex->shaderId());

	std::string fragment_error;
	size_t fragment_error_line;
	program_fragment = std::make_unique<EZCOGL::Shader>(GL_FRAGMENT_SHADER);
	if (!program_fragment->compile(fragment_source.get_source_code(), name, fragment_error, fragment_error_line))
	{
		size_t local_line;
		compilation_error = {
			.error = fragment_error,
			.line = 0,
			.file = fragment_source.get_file_at_line(fragment_error_line, local_line),
			.is_fragment = true,
		};
		compilation_error->line = local_line;
		glDeleteProgram(shader_program_id);
		shader_program_id = 0;
		is_dirty = false;
		return;
	}
	glAttachShader(shader_program_id, program_fragment->shaderId());

	glLinkProgram(shader_program_id);

	glDetachShader(shader_program_id, program_vertex->shaderId());
	glDetachShader(shader_program_id, program_fragment->shaderId());

	GL_CHECK_ERROR();
	int infologLength = 0;
	glGetProgramiv(shader_program_id, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1)
	{
		char* infoLog = new char[infologLength];
		int charsWritten = 0;
		glGetProgramInfoLog(shader_program_id, infologLength, &charsWritten, infoLog);
		compilation_error = {
			.error = infoLog,
			.line = 0,
			.file = "",
			.is_fragment = false,
		};
		delete[] infoLog;
		glDeleteProgram(shader_program_id);
		shader_program_id = 0;
		is_dirty = false;
		return;
	}
	if (compilation_error)
	{
		glDeleteProgram(shader_program_id);
		shader_program_id = 0;
		glUseProgram(0);
	}
	GL_CHECK_ERROR();

	const int world_data_id = glGetUniformBlockIndex(shader_program_id, "WorldData");
	if (world_data_id >= 0)
		glUniformBlockBinding(shader_program_id, world_data_id, 0);

	GL_CHECK_ERROR();
	int uniform_count;
	glGetProgramiv(shader_program_id, GL_ACTIVE_UNIFORMS, &uniform_count);
	for (int i = 0; i < uniform_count; ++i)
	{
		GLchar uniform_name[256];
		int length;
		int type_size;
		GLenum type_type;
		glGetActiveUniform(shader_program_id, static_cast<GLuint>(i), 256, &length, &type_size, &type_type,
		                   uniform_name);
		bindings.insert({uniform_name, glGetUniformLocation(shader_program_id, uniform_name)});
	}
	is_dirty = false;
	GL_CHECK_ERROR();
}

Material::~Material()
{
	auto& materials = Engine::get().get_asset_manager().materials;
	materials.erase(std::find(materials.begin(), materials.end(), this));
	glDeleteProgram(shader_program_id);
}

bool Material::bind()
{
	GL_CHECK_ERROR();
	if (is_dirty)
		reload_internal();

	if (compilation_error)
		return false;

	glUseProgram(shader_program_id);
	GL_CHECK_ERROR();
	return true;
}

void Material::set_model_transform(const Eigen::Affine3d& transformation)
{
	const int model_location = binding("model");
	if (model_location)
		glUniformMatrix4fv(model_location, 1, false, transformation.cast<float>().matrix().data());
}

void Material::load_from_source(const std::string& in_vertex_path, const std::string& in_fragment_path)
{
	vertex_source.set_source_path(in_vertex_path);
	fragment_source.set_source_path(in_fragment_path);
	mark_dirty();
}

void Material::check_updates()
{
	if (!auto_reload)
		return;

	vertex_source.check_update();
	fragment_source.check_update();

	if (is_dirty)
		reload_internal();
}
