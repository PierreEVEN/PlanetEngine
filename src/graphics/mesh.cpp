#include "mesh.h"

#include <iostream>
#include <GL/gl3w.h>

#include <utility>

Mesh::~Mesh()
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
}

void Mesh::set_positions(std::vector<Eigen::Vector3f> in_positions, int location, bool no_update)
{
	positions = std::move(in_positions);
	att_pos = location;
	if (!no_update)
		rebuild_mesh_data();
}

void Mesh::set_texture_coordinates(std::vector<Eigen::Vector2f> in_texture_coordinates, int location, bool no_update)
{
	texture_coordinates = std::move(in_texture_coordinates);
	att_text_coords = location;
	if (!no_update)
		rebuild_mesh_data();
}

void Mesh::set_normals(std::vector<Eigen::Vector3f> in_normals, int location, bool no_update)
{
	normals = std::move(in_normals);
	att_norms = location;
	if (!no_update)
		rebuild_mesh_data();
}

void Mesh::set_tangents(std::vector<Eigen::Vector3f> in_tangents, int location, bool no_update)
{
	tangents = std::move(in_tangents);
	att_tang = location;
	if (!no_update)
		rebuild_mesh_data();
}

void Mesh::set_colors(std::vector<Eigen::Vector3f> in_colors, int location, bool no_update)
{
	colors = std::move(in_colors);
	att_colors = location;
	if (!no_update)
		rebuild_mesh_data();
}

void Mesh::set_indices(std::vector<uint32_t> in_indices, bool no_update)
{
	indices = std::move(in_indices);
	if (!no_update)
		rebuild_mesh_data();
}

void Mesh::draw() const
{
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

Mesh::Mesh()
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
}

Mesh::Mesh(const std::string& path) : Mesh()
{
}

void Mesh::rebuild_mesh_data() const
{
	glBindVertexArray(vao);

	size_t structure_size = 0;
	size_t vertex_count = 0;
	if (att_pos >= 0)
	{
		structure_size += sizeof(Eigen::Vector3f);
		vertex_count = positions.size();
	}
	if (att_text_coords >= 0)
	{
		structure_size += sizeof(Eigen::Vector2f);
		vertex_count = texture_coordinates.size();
	}
	if (att_norms >= 0)
	{
		structure_size += sizeof(Eigen::Vector3f);
		vertex_count = normals.size();
	}
	if (att_tang >= 0)
	{
		structure_size += sizeof(Eigen::Vector3f);
		vertex_count = tangents.size();
	}
	if (att_colors >= 0)
	{
		structure_size += sizeof(Eigen::Vector3f);
		vertex_count = colors.size();
	}

	if (vertex_count == 0)
		return;

	auto* vertex_data_memory = new uint8_t[vertex_count * structure_size];

	for (uint32_t i = 0; i < vertex_count; ++i)
	{
		uint8_t* vertex_begin = &vertex_data_memory[structure_size * i];
		uint32_t current_offset = 0;
		if (att_pos >= 0)
		{
			*reinterpret_cast<Eigen::Vector3f*>(&vertex_begin[current_offset]) = positions[i];
			current_offset += sizeof(Eigen::Vector3f);
		}
		if (att_text_coords >= 0)
		{
			*reinterpret_cast<Eigen::Vector2f*>(&vertex_begin[current_offset]) = texture_coordinates[i];
			current_offset += sizeof(Eigen::Vector2f);
		}
		if (att_norms >= 0)
		{
			*reinterpret_cast<Eigen::Vector3f*>(&vertex_begin[current_offset]) = normals[i];
			current_offset += sizeof(Eigen::Vector3f);
		}
		if (att_tang >= 0)
		{
			*reinterpret_cast<Eigen::Vector3f*>(&vertex_begin[current_offset]) = tangents[i];
			current_offset += sizeof(Eigen::Vector3f);
		}
		if (att_colors >= 0)
		{
			*reinterpret_cast<Eigen::Vector3f*>(&vertex_begin[current_offset]) = colors[i];
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * structure_size, vertex_data_memory, GL_STATIC_DRAW);


	uint32_t current_offset = 0;
	if (att_pos >= 0)
	{
		glEnableVertexAttribArray(att_pos);
		glVertexAttribPointer(att_pos, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(structure_size),
		                      reinterpret_cast<void*>(static_cast<size_t>(current_offset)));
		current_offset += sizeof(Eigen::Vector3f);
	}
	if (att_text_coords)
	{
		glEnableVertexAttribArray(att_text_coords);
		glVertexAttribPointer(att_text_coords, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(structure_size),
		                      reinterpret_cast<void*>(static_cast<size_t>(current_offset)));
		current_offset += sizeof(Eigen::Vector2f);
	}
	if (att_norms >= 0)
	{
		glEnableVertexAttribArray(att_norms);
		glVertexAttribPointer(att_norms, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(structure_size),
		                      reinterpret_cast<void*>(static_cast<size_t>(current_offset)));
		current_offset += sizeof(Eigen::Vector3f);
	}
	if (att_tang >= 0)
	{
		glEnableVertexAttribArray(att_tang);
		glVertexAttribPointer(att_tang, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(structure_size),
		                      reinterpret_cast<void*>(static_cast<size_t>(current_offset)));
		current_offset += sizeof(Eigen::Vector3f);
	}
	if (att_colors >= 0)
	{
		glEnableVertexAttribArray(att_colors);
		glVertexAttribPointer(att_colors, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(structure_size),
		                      reinterpret_cast<void*>(static_cast<size_t>(current_offset)));
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size()) * sizeof(uint32_t), indices.data(),
	             GL_STATIC_DRAW);

	glBindVertexArray(0);
}
