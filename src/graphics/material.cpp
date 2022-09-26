#include "material.h"

Material::Material(const std::string& in_name) : name(in_name) {}

Material::~Material() {
	glDeleteProgram(shader_id);
}

void Material::use() {
	glUseProgram(shader_id);
}

void Material::set_model_transform(const Eigen::Affine3d& transformation)
{
	glUniformMatrix4fv(1, 1, false, transformation.cast<float>().matrix().data());
}

void Material::load_from_source(const char* vertex_path, const char* fragment_path) {

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
		std::cerr << "Link message :" << name << " :" << std::endl << infoLog << std::endl;
		delete[] infoLog;
	}

	glUseProgram(0);

	auto world_data_id = glGetUniformBlockIndex(shader_id, "WorldData");
	glUniformBlockBinding(shader_id, world_data_id, 0);
}