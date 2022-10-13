

#include "shader_source.h"

#include <filesystem>
#include <fstream>
#include <iostream>

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
	for (const auto& dep : content)
		if (dep->get_dependency())
			dep->get_dependency()->on_data_changed.clear();
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

	//@TODO Crash sometime here
	on_data_changed.execute();
}