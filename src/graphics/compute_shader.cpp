

#include "compute_shader.h"

#include <shader_program.h>
#include <GL/gl3w.h>

#include "texture_image.h"
#include "engine/asset_manager.h"
#include "engine/engine.h"

ComputeShader::~ComputeShader()
{
	auto& computes = Engine::get().get_asset_manager().compute_shaders;
	computes.erase(std::find(computes.begin(), computes.end(), this));
	glDeleteProgram(compute_shader_id);
}

void ComputeShader::load_from_source(const std::string& in_compute_path)
{
	compute_source.set_source_path(in_compute_path);
	mark_dirty();
}

void ComputeShader::check_updates()
{
	if (!auto_reload)
		return;
	compute_source.check_update();
}

void ComputeShader::bind()
{
	if (is_dirty)
		reload_internal();

	glUseProgram(compute_shader_id);
}

void ComputeShader::execute(int x, int y, int z)
{
	glDispatchCompute(x, y, z);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

ComputeShader::ComputeShader(const std::string& in_name) : name(in_name)
{
	Engine::get().get_asset_manager().compute_shaders.emplace_back(this);
	compute_source.on_data_changed.add_object(this, &ComputeShader::mark_dirty);

	compute_shader_id = glCreateProgram();
}

void ComputeShader::reload_internal()
{
	std::string fragment_error;
	size_t fragment_error_line;
	program_compute = std::make_unique<EZCOGL::Shader>(GL_COMPUTE_SHADER);
	if (!program_compute->compile(compute_source.get_source_code(), name, fragment_error, fragment_error_line))
	{
		size_t local_line;
		compilation_error = {
			.error = fragment_error,
			.line = 0,
			.file = compute_source.get_file_at_line(fragment_error_line, local_line),
			.is_fragment = true,
		};
		compilation_error->line = local_line;
		is_dirty = false;
		return;
	}
	glAttachShader(compute_shader_id, program_compute->shaderId());
	glLinkProgram(compute_shader_id);
	glDetachShader(compute_shader_id, program_compute->shaderId());

	int infologLength = 0;
	glGetProgramiv(compute_shader_id, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1)
	{
		char* infoLog = new char[infologLength];
		int charsWritten = 0;
		glGetProgramInfoLog(compute_shader_id, infologLength, &charsWritten, infoLog);
		compilation_error = {
			.error = infoLog,
			.line = 0,
			.file = "",
			.is_fragment = false,
		};
		delete[] infoLog;
		glDeleteProgram(compute_shader_id);
		compute_shader_id = 0;
		is_dirty = false;
		return;
	}
	if (compilation_error)

		glUseProgram(0);

	glUniformBlockBinding(compute_shader_id, glGetUniformBlockIndex(compute_shader_id, "WorldData"), 0);

	int uniform_count;
	glGetProgramiv(compute_shader_id, GL_ACTIVE_UNIFORMS, &uniform_count);
	for (int i = 0; i < uniform_count; ++i)
	{
		GLchar uniform_name[256];
		int length;
		int type_size;
		GLenum type_type;
		glGetActiveUniform(compute_shader_id, static_cast<GLuint>(i), 256, &length, &type_size, &type_type, uniform_name);
		bindings.insert({ uniform_name, glGetUniformLocation(compute_shader_id, uniform_name) });
	}
	is_dirty = false;
}
