#pragma once

#include <optional>
#include <GL/gl3w.h>

#include "world.h"
#include <utils/event_manager.h>

#include "shader_source.h"

namespace EZCOGL
{
	class Shader;
}

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

	std::optional<CompilationErrorInfo> compilation_error;

	[[nodiscard]] int binding(const std::string& binding_name) const;
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