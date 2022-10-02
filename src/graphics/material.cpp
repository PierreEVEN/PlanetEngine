#include "material.h"

#include <filesystem>
#include <GL/gl3w.h>

#include <shader_program.h>

#include "engine/asset_manager.h"
#include "engine/engine.h"

Material::Material(const std::string& in_name) : name(in_name)
{
	Engine::get().get_asset_manager().materials.emplace_back(this);
}

Material::~Material()
{
	auto& materials = Engine::get().get_asset_manager().materials;
	materials.erase(std::ranges::find(materials, this));
	glDeleteProgram(shader_id);
}

void Material::use()
{
	if (auto_reload && (last_vertex_update != std::filesystem::last_write_time(vertex_path) || last_fragment_update !=
		std::filesystem::last_write_time(fragment_path)))
		hot_reload();

	glUseProgram(shader_id);
}

void Material::set_model_transform(const Eigen::Affine3d& transformation)
{
	glUniformMatrix4fv(1, 1, false, transformation.cast<float>().matrix().data());
}

void Material::load_from_source(const std::string& in_vertex_path, const std::string& in_fragment_path)
{
	vertex_path = in_vertex_path;
	fragment_path = in_fragment_path;
	last_vertex_update = std::filesystem::last_write_time(vertex_path);
	last_fragment_update = std::filesystem::last_write_time(fragment_path);
	// Compile shader
	shader_id = glCreateProgram();

	program_vertex = std::make_unique<EZCOGL::Shader>(GL_VERTEX_SHADER);
	program_vertex->compile(EZCOGL::load_src(vertex_path), name);
	glAttachShader(shader_id, program_vertex->shaderId());

	program_fragment = std::make_unique<EZCOGL::Shader>(GL_FRAGMENT_SHADER);
	program_fragment->compile(EZCOGL::load_src(fragment_path), name);
	glAttachShader(shader_id, program_fragment->shaderId());

	glLinkProgram(shader_id);

	glDetachShader(shader_id, program_vertex->shaderId());
	glDetachShader(shader_id, program_fragment->shaderId());


	int infologLength = 0;
	glGetProgramiv(shader_id, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1)
	{
		char* infoLog = new char[infologLength];
		int charsWritten = 0;
		glGetProgramInfoLog(shader_id, infologLength, &charsWritten, infoLog);
		last_error = infoLog;
		if (infologLength > 0 && infoLog[0] == 'F')
			last_error = last_error + "\nfile : " + fragment_path;
		else
			last_error = last_error +"\nfile : " + vertex_path;
		std::cerr << "Link message :" << name << " :" << std::endl << infoLog << std::endl;
		delete[] infoLog;
		shader_id = NULL;
	}

	glUseProgram(0);

	auto world_data_id = glGetUniformBlockIndex(shader_id, "WorldData");
	glUniformBlockBinding(shader_id, world_data_id, 0);
}

void Material::hot_reload()
{
	load_from_source(vertex_path, fragment_path);
}
