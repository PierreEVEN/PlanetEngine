#include "material.h"

#include <filesystem>
#include <fstream>
#include <GL/gl3w.h>

#include <shader_program.h>

#include "engine/asset_manager.h"
#include "engine/engine.h"
#include "utils/profiler.h"

int Material::binding(const std::string& binding_name) const
{
	const auto binding = bindings.find(binding_name);
	if (binding != bindings.end())
		return binding->second;
	return -1;
}

Material::Material(const std::string& in_name) : name(in_name)
{
	Engine::get().get_asset_manager().materials.emplace_back(this);
	vertex_source.on_data_changed.add_object(this, &Material::mark_dirty);
	fragment_source.on_data_changed.add_object(this, &Material::mark_dirty);
}

void Material::reload_internal()
{
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

		glUseProgram(0);

	glUniformBlockBinding(shader_program_id, glGetUniformBlockIndex(shader_program_id, "WorldData"), 0);

	int uniform_count;
	glGetProgramiv(shader_program_id, GL_ACTIVE_UNIFORMS, &uniform_count);
	for (int i = 0; i < uniform_count; ++i)
	{
		GLchar uniform_name[256];
		int length;
		int type_size;
		GLenum type_type;
		glGetActiveUniform(shader_program_id, static_cast<GLuint>(i), 256, &length, &type_size, &type_type, uniform_name);
		bindings.insert({ uniform_name, glGetUniformLocation(shader_program_id, uniform_name) });
	}
	is_dirty = false;
}

Material::~Material()
{
	auto& materials = Engine::get().get_asset_manager().materials;
	materials.erase(std::find(materials.begin(), materials.end(), this));
	glDeleteProgram(shader_program_id);
}

void Material::bind()
{
	if (is_dirty)
		reload_internal();

	glUseProgram(shader_program_id);
}

void Material::set_model_transform(const Eigen::Affine3d& transformation)
{
	glUniformMatrix4fv(1, 1, false, transformation.cast<float>().matrix().data());
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
}


ShaderSource::ShaderSource() { last_file_update = new std::filesystem::file_time_type(); }
ShaderSource::~ShaderSource() { delete static_cast<std::filesystem::file_time_type*>(last_file_update); }

std::string ShaderSource::get_source_code() const
{
	std::string source_code;
	for (const auto& item : content)
		source_code += item->get_content();
	return source_code;
}

void ShaderSource::check_update()
{
	// Ensure file exists
	if (source_path.empty())
	{
		if (!content.empty())
		{
			content.clear();
			on_data_changed.execute();
		}
		return;
	}

	if (!std::filesystem::exists(source_path))
	{
		return;
	}

	// Check if change have been made
	if (*static_cast<std::filesystem::file_time_type*>(last_file_update) ==
		std::filesystem::last_write_time(source_path))
	{
		// No changes here, but check dependencies anyway
		for (size_t i = 0; i < content.size(); ++i)
		{
			content[i]->check_update();
		}
		return;
	}

	// Changes detected here : reload file and dependencies

	reload_internal();
}

void ShaderSource::set_source_path(const std::string& in_source_path)
{
	if (source_path == in_source_path)
		return;
	source_path = in_source_path;
	reload_internal();
}

std::string ShaderSource::get_file_name() const
{
	return std::filesystem::path(source_path).filename().string();
}

std::vector<const ShaderSource*> ShaderSource::get_dependencies() const
{
	std::vector<const ShaderSource*> dependencies;
	for (const auto& dep : content)
		if (const auto* source = dep->get_dependency())
			dependencies.emplace_back(source);
	return dependencies;
}

size_t ShaderSource::get_line_count() const
{
	size_t line_count = 0;
	for (const auto& dep : content)
		line_count += dep->get_line_count();
	return line_count;
}

std::string ShaderSource::get_file_at_line(size_t line, size_t& local_line) const
{
	std::string file = get_path();

	size_t current_start = 0;
	size_t include_size = 0;
	for (const auto& chunk : content)
	{
		if (line < current_start + chunk->get_line_count())
		{
			if (const auto* dep = chunk->get_dependency())
				return dep->get_file_at_line(line - current_start, local_line);
			local_line = line - include_size;
			return file;
		}
		if (chunk->get_dependency())
			include_size += chunk->get_line_count();
		current_start += chunk->get_line_count();
	}

	local_line = line - include_size;
	return file;
}

void ShaderSource::reload_internal()
{
	if (!std::filesystem::exists(source_path))
	{
		std::cerr << "file " << source_path.c_str() << " does not exists";
		return;
	}

	// Update recorded timestamp
	*static_cast<std::filesystem::file_time_type*>(last_file_update) = std::filesystem::last_write_time(source_path);

	// Unlink dependencies
	content.clear();

	// Open file
	std::ifstream file(source_path, std::ios_base::in);
	if (!file.is_open())
	{
		return;
	}

	std::string shader_text_code;
	size_t line_count = 0;
	// Read file line by line
	for (std::string line; std::getline(file, line);)
	{
		bool is_include = false;
		for (size_t i = 0; i < line.length(); ++i)
		{
			if (line.substr(i, 8) == "#include")
			{
				// We encountered include directive. Store parsed data into new chunk.
				if (!shader_text_code.empty())
					content.emplace_back(std::make_shared<SourceChunkText>(shader_text_code, line_count));
				shader_text_code.clear();
				line_count = 0;

				is_include = true;

				std::string include_path;
				// move to next '"'
				for (++i; i < line.length() && line[i] != '"'; ++i);
				// read until next '"'
				for (++i; i < line.length() && line[i] != '"'; ++i)
					include_path += line[i];

				include_path = std::filesystem::path(get_path()).parent_path().concat("/").concat(include_path).
				                                                 string();
				const auto new_dep = std::make_shared<SourceChunkDependency>();
				new_dep->dependency.set_source_path(include_path);
				new_dep->dependency.on_data_changed.add_object(this, &ShaderSource::reload_internal);
				content.emplace_back(new_dep);
				break;
			}
			// Not an include directive
			if (!std::isblank(line[i]))
				break;
		}
		if (!is_include)
		{
			shader_text_code += line + '\n';
			line_count++;
		}
	}

	if (!shader_text_code.empty())
		content.emplace_back(std::make_shared<SourceChunkText>(shader_text_code, line_count));
	shader_text_code.clear();
	line_count = 0;

	on_data_changed.execute();
}
