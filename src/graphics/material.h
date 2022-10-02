#pragma once

#include <filesystem>

#include "world.h"

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

	void use();
	void set_model_transform(const Eigen::Affine3d& transformation);

	void load_from_source(const std::string& vertex_path, const std::string& fragment_path);
	void hot_reload();

	[[nodiscard]] const std::string& get_vertex_path() const { return vertex_path; }
	[[nodiscard]] const std::string& get_fragment_path() const { return fragment_path; }

	[[nodiscard]] uint32_t program_id() const{ return shader_id; }

	const std::string name;
	bool auto_reload = false;
	std::string last_error;
private:
	Material(const std::string& name);

	std::unique_ptr<EZCOGL::Shader> program_vertex;
	std::unique_ptr<EZCOGL::Shader> program_fragment;
	uint32_t shader_id;
	std::string vertex_path;
	std::string fragment_path;
	std::filesystem::file_time_type last_vertex_update;
	std::filesystem::file_time_type last_fragment_update;
};
