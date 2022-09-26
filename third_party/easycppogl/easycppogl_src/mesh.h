/*******************************************************************************
* EasyCppOGL:   Copyright (C) 2019,                                            *
* Sylvain Thery, IGG Group, ICube, University of Strasbourg, France            *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Contact information: thery@unistra.fr                                        *
*******************************************************************************/


#ifndef EASY_CPP_OGL_MESH_H_
#define EASY_CPP_OGL_MESH_H_

#include <memory>
#include <tuple>
#include <vector>
#include <gl_eigen.h>
#include <vao.h>
#include <ebo.h>
#include <texture2d.h>
#include <boundingbox.h>
#include <scenegraph.h>
#include <material.h>

namespace EZCOGL
{

class MeshRenderer
{
	friend class Mesh;
protected:
	VAO::UP vao_;
	std::size_t nb_vert_;
	EBO::SP ebo_triangles_;
    EBO::SP ebo_lines_;
    BoundingBox::SP bb_;
	Material::SP mat_;

	MeshRenderer(const Mesh& m, GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col);
public:
	using UP = std::unique_ptr<MeshRenderer>;
	~MeshRenderer();
	MeshRenderer(const MeshRenderer&) = delete;
	void draw(GLenum prim) const ;
    inline const BoundingBox::SP BB() const { return bb_; }
	inline const Material::SP material() const { return mat_; }
	inline bool add_vbos(const std::vector<std::tuple<GLint, VBO::SP>>& att_vbo)
	{
		for( const auto& p: att_vbo)
			if (vao_->use_loc(std::get<0>(p)))
			{
				std::cerr << "Warning location "<< std::get<0>(p) << "already used in this VAO"<< std::endl;
				return false;
			}
		vao_->add(att_vbo);
		return true;
	}

};

class InstancedMeshRenderer
{
	friend class Mesh;
protected:
	VAO::UP vao_;
	GLuint nb_vert_;
	EBO::SP ebo_triangles_;
    EBO::SP ebo_lines_;
    BoundingBox::SP bb_;
	Material::SP mat_;
	InstancedMeshRenderer(const Mesh& m, const std::vector<std::tuple<GLint, VBO::SP, GLint>>& inst_vbos, GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col);
public:
	using UP = std::unique_ptr<InstancedMeshRenderer>;
	~InstancedMeshRenderer();
	InstancedMeshRenderer(const InstancedMeshRenderer&) = delete;
	void draw(GLenum prim, GLuint nb);
	inline const BoundingBox::SP BB() const { return bb_; }
	inline const Material::SP material() const { return mat_; }
};

class MeshAssimpImporter;
	
class Mesh
{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	friend class MeshAssimpImporter;
	friend class MeshRenderer;
	friend class InstancedMeshRenderer;
public:
	using SP = std::shared_ptr<Mesh>;

protected:
	std::vector<GLVec3> vertices_;
	std::vector<GLVec3> normals_;
	std::vector<GLVec2> tex_coords_;
	std::vector<GLVec3> tangents_;
	std::vector<GLVec3> colors_;
	std::vector<GLuint> tri_indices_;
	std::vector<GLuint> line_indices_;
	Material::SP mat_;
	std::string name_ = "unknown";
	BoundingBox::SP bb_;
protected:
	inline Mesh()
	{
		bb_ = BoundingBox::create();
		mat_ = std::make_shared<Material>();
	}

	void grid_topo(GLint m, GLint n);

public:
	
	Mesh(const Mesh&) = delete;
	Mesh(Mesh&& m);

	void compute_normals();

	inline const std::string& name()
	{
		return name_;
	}

	inline bool has_positions() const { return !vertices_.empty(); }
	inline bool has_tex_coords() const { return !tex_coords_.empty(); }
	inline bool has_normals() const { return !normals_.empty(); }
	inline bool has_tangents() const { return !tangents_.empty(); }
	inline bool has_colors() const { return !colors_.empty(); }

	inline std::size_t nb_vertices() const
	{
		return vertices_.size();
	}

	inline std::size_t nb_triangles() const
	{
		return tri_indices_.size() / 3;
	}

	inline std::vector<GLuint> line_indices() const
	{
		return line_indices_;
	}

	inline std::vector<GLuint> tri_indices() const
	{
		return tri_indices_;
	}

	inline std::size_t nb_lines() const
	{
		return line_indices_.size() / 3;
	}


	inline const GLVVec3& vertices()
	{
		return vertices_;
	}

	inline const GLVVec3& normals()
	{
		return normals_;
	}

	inline const GLVVec2& tex_coords()
	{
		return tex_coords_;
	}

	inline const std::vector<GLVec3>& tangents()
	{
		return tangents_;
	}

	inline const std::vector<GLVec3>& colors()
	{
		return colors_;
	}

	inline Material::SP material() { return mat_ ; }
	inline const Material::SP material() const { return mat_; }

	inline const BoundingBox::SP BB() const { return bb_;}

	MeshRenderer::UP renderer(GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col) const;

	InstancedMeshRenderer::UP instanced_renderer(const std::vector<std::tuple<GLint, VBO::SP, GLint>>& inst_vbos,GLint att_pos, GLint att_norm, GLint att_tc, GLint att_tang, GLint att_col) const;

	static Mesh::SP CubePosOnly();
	static Mesh::SP Cube();
    static Mesh::SP Grid(GLint m=4, GLint n=4);
    static Mesh::SP Wave(GLint m);
	static Mesh::SP Sphere( GLint n);
    static Mesh::SP Cylinder(GLint m, GLint n, float radius);
    static Mesh::SP Tore(GLint m, GLint n, float radius_ratio);

	static typename SceneGraphNode<Mesh::SP>::SP load(const std::string& mesh_filename);

	void transform(const GLMat4& trf)
	{
		for (GLVec3& v : vertices_)
			v = Transfo::apply(trf, v);

		GLMat3 trfn = Transfo::inverse_transpose(trf);

		for (GLVec3& n : normals_)
			n = Transfo::apply(trfn, n);

		for (GLVec3& t : tangents_)
			t = Transfo::apply(trfn, t);
	}

};

template <typename T>
int SceneGraphNode<T>::next_id = 0;


inline BoundingBox::SP compute_BB_SceneGraph(const SceneGraphNode<Mesh::SP>& sg)
{
	BoundingBox::SP bb = BoundingBox::create();
	sg.traverse_data([&bb](const Mesh& m, const GLMat4 trf) { bb->merge(*(m.BB()->transformed(trf))); });
	return bb;
}

inline void flatten_sg(SceneGraphNode<Mesh::SP>& sg)
{
	sg.flatten([](Mesh& m, const GLMat4& trf) { m.transform(trf); });
}



}
#endif
