#pragma once

#include <optional>
#include <GL/gl3w.h>

#include "world.h"
#include <utils/event_manager.h>

class ShaderSource;
DECLARE_DELEGATE_MULTICAST(Event_ShaderFileUpdate);

namespace EZCOGL
{
	class Shader;
}


class ISourceChunk
{
public:
	virtual std::string get_content() = 0;
	virtual void check_update() = 0;
	virtual ShaderSource* get_dependency() = 0;
	virtual size_t get_line_count() = 0;
};

class ShaderSource final
{
public:
	ShaderSource();
	~ShaderSource();
	[[nodiscard]] std::string get_source_code() const;
	Event_ShaderFileUpdate on_data_changed;

	// Reload file if changes have been detected
	void check_update();
	void set_source_path(const std::string& source_path);

	[[nodiscard]] const std::string& get_path() const { return source_path; }
	[[nodiscard]] std::string get_file_name() const;

	std::vector<const ShaderSource*> get_dependencies() const;

	[[nodiscard]] size_t get_line_count() const;

	std::string get_file_at_line(size_t line, size_t& local_line) const;

private:
	std::vector<std::shared_ptr<ISourceChunk>> content;

	std::string source_path;
	void* last_file_update = nullptr;

	void child_reloaded();
	void reload_internal();
};

class SourceChunkText : public ISourceChunk
{
public:
	std::string get_content() override { return text; }
	SourceChunkText(std::string in_text, size_t line_count) : text(std::move(in_text)), line_count(line_count) { return; }
	void check_update() override { return; }
	ShaderSource* get_dependency() override { return nullptr; }
	size_t get_line_count() override { return line_count; }
private:
	size_t line_count = 0;
	const std::string text;
};

class SourceChunkDependency : public ISourceChunk
{
public:
	std::string get_content() override { return dependency.get_source_code(); }
	void check_update() override { dependency.check_update(); }
	ShaderSource* get_dependency() override { return &dependency; }
	size_t get_line_count() override { return dependency.get_line_count(); }
	ShaderSource dependency;
};

class Material final
{
public:
	~Material();

	static std::shared_ptr<Material> create(const std::string& name)
	{
		return std::shared_ptr<Material>(new Material(name));
	}

	void bind();
	void set_model_transform(const Eigen::Affine3d& transformation);

	// Load or reload shader
	void load_from_source(const std::string& vertex_path, const std::string& fragment_path);
	void check_updates();

	[[nodiscard]] const ShaderSource& get_vertex_source() const { return vertex_source; }
	[[nodiscard]] const ShaderSource& get_fragment_source() const { return fragment_source; }

	[[nodiscard]] uint32_t program_id() const { return shader_program_id; }

	const std::string name;
	bool auto_reload = false;

	struct CompilationErrorInfo
	{
		std::string error;
		size_t line;
		std::string file;
		bool is_fragment;
	};
	std::optional<CompilationErrorInfo> compilation_error;

	int binding(const std::string& binding_name) const;
private:

	Material(const std::string& name);

	// GL Handles
	std::unique_ptr<EZCOGL::Shader> program_vertex;
	std::unique_ptr<EZCOGL::Shader> program_fragment;
	uint32_t shader_program_id;

	// Source file paths
	ShaderSource vertex_source;
	ShaderSource fragment_source;

	void reload_internal();

	void mark_dirty() { is_dirty = true; }
	bool is_dirty;

	std::unordered_map<std::string, int> bindings;
};