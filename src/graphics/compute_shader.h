#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "shader_source.h"

class TextureBase;

namespace EZCOGL
{
	class Shader;
}

enum class BindingMode
{
	In,
	Out,
	InOut
};

class ComputeShader
{
public:
	~ComputeShader();

	static std::shared_ptr<ComputeShader> create(const std::string& name)
	{
		return std::shared_ptr<ComputeShader>(new ComputeShader(name));
	}

	// Load or reload shader
	void load_from_source(const std::string& compute_path);
	void check_updates();

	[[nodiscard]] uint32_t program_id() const { return compute_shader_id; }
	[[nodiscard]] ShaderSource& get_program_source() { return compute_source; }

	const std::string name;
	std::optional<CompilationErrorInfo> compilation_error;
	bool auto_reload = false;

	void bind();
	void execute(int x, int y, int z);

	void bind_texture(const std::shared_ptr<TextureBase>& texture, BindingMode mode, int32_t binding);

private:

	ComputeShader(const std::string& in_name);
	ShaderSource compute_source;

	void reload_internal();
	void mark_dirty() { is_dirty = true; }
	bool is_dirty = true;
	uint32_t compute_shader_id;
	std::unique_ptr<EZCOGL::Shader> program_compute;
	std::unordered_map<std::string, int> bindings;
};
