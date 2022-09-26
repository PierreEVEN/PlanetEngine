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


#ifndef EASY_CPP_OGL_MESH_ASSIMP_IMPORTER_H_
#define EASY_CPP_OGL_MESH_ASSIMP_IMPORTER_H_

#include <gl_eigen.h>
#include <mesh.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>

struct aiMesh;
struct aiMaterial;
struct aiNode;
struct aiScene;

namespace EZCOGL
{


class MeshAssimpImporter
{
public:
	using SGNM = SceneGraphNode<Mesh::SP>;
	SGNM::SP meshes_;
	std::string path_;
	std::string fname_;
	std::map < std::string , Texture2D::SP> tex_names_;

	SGNM::SP ai_process_node(aiNode* node, const aiScene* scene);
	Mesh::SP create_mesh(::aiMesh* aimesh, ::aiMaterial* aimaterial);
	void load_tex(aiMaterial* aimaterial, aiTextureType tt, Texture2D::SP& tex);
	static const std::map<int, GLenum> mapwrap_s_;

public:
	MeshAssimpImporter(const std::string& mesh_filename);
	void check_aiVertexNormals(aiNode* node, const aiScene* scene);
	inline SGNM::SP get_sg_meshes()
	{
		return std::move(meshes_);
	}

	inline SGNM::SP get_meshes()
	{
		flatten_sg(*meshes_);
		return std::move(meshes_);
	}
};

}
#endif
