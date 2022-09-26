#pragma once

#include "../world.h"
#include <shader_program.h>

class Material final {
public:

	~Material();

	static std::shared_ptr<Material> create(const std::string& name) {
		return std::shared_ptr<Material>(new Material(name));
	}

	void use();
	void set_model_transform(const Eigen::Affine3d& transformation);

	void load_from_source(const char* vertex_path, const char* fragment_path);

	uint32_t program_id() const {
		shader_id;
	}

private:
	Material(const std::string& name);

	std::unique_ptr<EZCOGL::Shader> program_vertex;
	std::unique_ptr<EZCOGL::Shader> program_fragment;
	uint32_t shader_id;
	std::string name;
};