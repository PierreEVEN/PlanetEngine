#pragma once

#include "world.h"
#include <utils/event_manager.h>

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
	SourceChunkText(const std::string& in_text) : text(in_text) { return; }
	void check_update() override { return; }
private:
	const std::string text;
};

class SourceChunkDependency : public ISourceChunk
{
public:
	std::string get_content() override { return dependency.get_source_code(); }
	void check_update() override { dependency.check_update(); }
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
};
