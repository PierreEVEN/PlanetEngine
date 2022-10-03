#pragma once

#include <memory>
#include <string>

#include <Eigen/Dense>

struct Vertex
{
	Eigen::Vector3f pos;
	Eigen::Vector2f text_coord;
	Eigen::Vector4f col;
	Eigen::Vector3f norm;
	Eigen::Vector3f tang;
};

class Mesh final
{
public:
	~Mesh();

	static std::shared_ptr<Mesh> create(const std::string& name)
	{
		return std::shared_ptr<Mesh>(new Mesh(name));
	}

	static std::shared_ptr<Mesh> from_file(const std::string& path)
	{
		return std::shared_ptr<Mesh>(new Mesh(path));
	}

	void set_positions(std::vector<Eigen::Vector3f> positions, int location, bool no_update = false);
	void set_texture_coordinates(std::vector<Eigen::Vector2f> texture_coordinates, int location, bool no_update = false);
	void set_normals(std::vector<Eigen::Vector3f> normals, int location, bool no_update = false);
	void set_tangents(std::vector<Eigen::Vector3f> tangents, int location, bool no_update = false);
	void set_colors(std::vector<Eigen::Vector3f> colors, int location, bool no_update = false);

	void set_indices(std::vector<uint32_t> indices, bool no_update = false);

	void draw() const;

	void rebuild_mesh_data() const;

	[[nodiscard]] size_t index_count() const { return indices.size(); }
	[[nodiscard]] size_t vertex_count() const { return positions.size(); }
	[[nodiscard]] uint32_t vao_id() const { return vao; }
	[[nodiscard]] uint32_t vbo_id() const { return vbo; }
	[[nodiscard]] uint32_t ebo_id() const { return ebo; }

	const std::string name;

private:
	Mesh(const std::string& name);
	Mesh(const std::string& name, const std::string& path);

	uint32_t vao;
	uint32_t vbo;
	uint32_t ebo;

	int att_pos = -1;
	int att_text_coords = -1;
	int att_norms = -1;
	int att_colors = -1;
	int att_tang = -1;

	std::vector<Eigen::Vector3f> positions;
	std::vector<Eigen::Vector2f> texture_coordinates;
	std::vector<Eigen::Vector3f> normals;
	std::vector<Eigen::Vector3f> tangents;
	std::vector<Eigen::Vector3f> colors;
	std::vector<uint32_t> indices;
};
