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

#include <mesh_assimp_importer.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>

#include <limits>
#include <cassert>
#include <condition_variable>


namespace EZCOGL
{

MeshAssimpImporter::MeshAssimpImporter(const std::string& mesh_filename)
{
	Assimp::Importer importer;
		importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE,70.0f);
	const aiScene* scene = importer.ReadFile(mesh_filename, aiProcess_Triangulate);
	check_aiVertexNormals(scene->mRootNode, scene);
	scene = importer.ApplyPostProcessing(aiProcess_GenSmoothNormals);
	scene = importer.ApplyPostProcessing(aiProcess_CalcTangentSpace | aiProcess_GenUVCoords);

	//const aiScene* scene =
	//		importer.ReadFile(mesh_filename, aiProcess_Triangulate | aiProcessPreset_TargetRealtime_Quality);


	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	auto mfn = mesh_filename;
	for (auto& c : mfn)
		if (c == '\\')
			c = '/';

	path_ = mfn.substr(0, mfn.find_last_of('/'));
	meshes_ = this->ai_process_node(scene->mRootNode, scene);
	std::cout << "TRF " << std::endl << (meshes_->transfo()) << std::endl;
}

SceneGraphNode<Mesh::SP>::SP MeshAssimpImporter::ai_process_node(::aiNode* node, const ::aiScene* scene)
{
	alignas(32) GLMat4 node_trf;
	node_trf.setIdentity();
	node_trf << node->mTransformation.a1, node->mTransformation.b1, node->mTransformation.c1, node->mTransformation.d1,
		node->mTransformation.a2, node->mTransformation.b2, node->mTransformation.c2, node->mTransformation.d2,
		node->mTransformation.a3, node->mTransformation.b3, node->mTransformation.c3, node->mTransformation.d3,
		node->mTransformation.a4, node->mTransformation.b4, node->mTransformation.c4, node->mTransformation.d4;

	SGNM::SP sgnode = SGNM::create(node_trf);
	BoundingBox::SP bb = sgnode->BB();
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		::aiMesh* aimesh = scene->mMeshes[node->mMeshes[i]];
		::aiMaterial* aimaterial = NULL;
		if (aimesh->mMaterialIndex >= 0)
			aimaterial = scene->mMaterials[aimesh->mMaterialIndex];

		Mesh::SP nm = create_mesh(aimesh, aimaterial);
		sgnode->add_data(nm);
		if (nm->BB()->is_initialized())
			bb->merge(*(nm->BB()));
	}
	if (bb->is_initialized())
		bb->transform(node_trf);

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		auto no = ai_process_node(node->mChildren[i], scene);
		sgnode->add_child(no);
		if (no->BB())
			bb->merge(*(no->BB()));
	}

	return sgnode;
}


const std::map<int, GLenum> MeshAssimpImporter::mapwrap_s_ =
	std::map<int, GLenum>{{aiTextureMapMode_Clamp, GL_CLAMP_TO_EDGE},
						{aiTextureMapMode_Decal, GL_CLAMP_TO_BORDER},
						{aiTextureMapMode_Wrap, GL_REPEAT},
						{aiTextureMapMode_Mirror, GL_MIRRORED_REPEAT}};



void MeshAssimpImporter::load_tex(::aiMaterial* aimaterial, ::aiTextureType tt, Texture2D::SP& tex)
{
	if (aimaterial->GetTextureCount(tt) > 0)
	{
		aiString fname;
		aiTextureMapMode mm = aiTextureMapMode_Wrap;
//		aimaterial->GetTexture(tt, 0, &fname, nullptr, nullptr, nullptr, nullptr, &mm);
		aimaterial->GetTexture(tt, 0, &fname);
		auto stfn = std::string(fname.C_Str());
		for (auto& c : stfn)
			if (c == '\\')
				c = '/';
		auto it = tex_names_.find(stfn);
		if (it == tex_names_.end())
		{
			GLenum wm = mapwrap_s_.at(int(mm));
			auto t = Texture2D::create({wm, GL_LINEAR_MIPMAP_LINEAR});
			bool res = t->load(path_ + "/" + stfn);
			if (!res)
			{
				size_t lp = stfn.find_last_of('.');

				for (size_t i = 0; i < lp; ++i)
				{
					auto& c = stfn[i];
					if ((c >= 'A') && (c <= 'Z'))
						c += 'a' - 'A';
				}
				res = t->load(path_ + "/" + stfn);
				if (!res)
				{
					for (size_t i = lp + 1; i < stfn.length(); ++i)
					{
						auto& c = stfn[i];
						if ((c >= 'A') && (c <= 'Z'))
							c += 'a' - 'A';
					}
					res = t->load(path_ + "/" + stfn);
				}
			}
			if (res)
			{
				tex = t;
				tex_names_[std::string(fname.C_Str())] = t;
				std::cout << std::string(fname.C_Str()) << std::endl;
			}
			else
			{
				std::cerr << "Failed loading Texture " << path_ + "/" + stfn << std::endl;
			}
		}
		else
		{
			tex = it->second;
//			std::cout << "using Texture " << it->first << std::endl;
		}
	}
}



void MeshAssimpImporter::check_aiVertexNormals(::aiNode* node, const ::aiScene* scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* pMesh = scene->mMeshes[node->mMeshes[i]];

		if (!pMesh->HasNormals() && (pMesh->mNumVertices <2 * pMesh->mNumFaces)) 
		{
			pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];

			for (unsigned int a = 0; a < pMesh->mNumVertices; a++)
				pMesh->mNormals[a].Set(0, 0, 0);

			for (unsigned int a = 0; a < pMesh->mNumFaces; a++)
			{
				const aiFace& face = pMesh->mFaces[a];
				for (unsigned int i = 0; i < face.mNumIndices; ++i)
				{
					const aiVector3D& pA = pMesh->mVertices[face.mIndices[(i + face.mNumIndices - 1)%face.mNumIndices]];
					const aiVector3D& pB = pMesh->mVertices[face.mIndices[i]];
					const aiVector3D& pC = pMesh->mVertices[face.mIndices[(i + 1) % face.mNumIndices]];
					aiVector3D BC = (pC - pB).Normalize();
					aiVector3D BA = (pA - pB).Normalize();
					//const aiVector3D vNor = (BC ^ BA);
					 float area = (BC ^ BA).Length();
					 if (area > 1.0)
						area = 3.1415926f-std::asin(1.0f-area);
					 else
						area = std::asin(area);
					 const aiVector3D vNor = (BC ^ BA).NormalizeSafe() * area;
					pMesh->mNormals[face.mIndices[i]] += vNor;
				}
			}
			for (unsigned int a = 0; a < pMesh->mNumVertices; a++)
				pMesh->mNormals[a] = pMesh->mNormals[a].NormalizeSafe();
		}
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		check_aiVertexNormals(node->mChildren[i], scene);
	}
}

Mesh::SP MeshAssimpImporter::create_mesh(::aiMesh* aimesh, ::aiMaterial* aimaterial)
{
	Mesh::SP mesh = std::shared_ptr<Mesh>(new Mesh());
	mesh->bb_ = BoundingBox::create();
	mesh->mat_ = std::make_shared<Material>();
	mesh->name_ = std::string(aimesh->mName.C_Str());
	if (mesh->name_.empty())
		mesh->name_ = "unknown";
	
	auto& vertices = mesh->vertices_;
	auto& tri_indices = mesh->tri_indices_;
	auto& line_indices = mesh->line_indices_;

	if (aimesh->HasPositions())
	{
		vertices.reserve(aimesh->mNumVertices);
		for (GLuint i = 0; i < aimesh->mNumVertices; ++i)
		{
			GLVec3 P(aimesh->mVertices[i].x, aimesh->mVertices[i].y, aimesh->mVertices[i].z);
			mesh->bb_->add_point(P);
			vertices.push_back(P);
		}
	}
	if (aimesh->HasNormals())
	{
		mesh->normals_.reserve(aimesh->mNumVertices);
		for (GLuint i = 0; i < aimesh->mNumVertices; ++i)
			mesh->normals_.push_back(GLVec3(aimesh->mNormals[i].x, aimesh->mNormals[i].y, aimesh->mNormals[i].z));
	}
	if (aimesh->HasTextureCoords(0))
	{
		mesh->tex_coords_.reserve(aimesh->mNumVertices);
		for (GLuint i = 0; i < aimesh->mNumVertices; ++i)
			mesh->tex_coords_.push_back(GLVec2(aimesh->mTextureCoords[0][i].x, aimesh->mTextureCoords[0][i].y));
	}

	if (aimesh->HasTangentsAndBitangents())
	{
		mesh->tangents_.reserve(aimesh->mNumVertices);
		for (GLuint i = 0; i < aimesh->mNumVertices; ++i)
			mesh->tangents_.push_back(GLVec3(aimesh->mTangents[i].x, aimesh->mTangents[i].y, aimesh->mTangents[i].z));
	}

	std::vector<std::vector<GLuint>> accel;

	if (aimesh->HasFaces())
	{

		accel.resize(vertices.size());
		for (auto& a : accel)
			a.reserve(8);

		GLuint nb_tri_ind = aimesh->mNumFaces * 3;
		tri_indices.reserve(nb_tri_ind);

		for (GLuint i = 0; i < aimesh->mNumFaces; ++i)
		{
			if (aimesh->mFaces[i].mNumIndices == 3)
			{
				auto A = aimesh->mFaces[i].mIndices[0];
				auto B = aimesh->mFaces[i].mIndices[1];
				auto C = aimesh->mFaces[i].mIndices[2];

				tri_indices.push_back(A);
				tri_indices.push_back(B);
				tri_indices.push_back(C);

				if (std::find(accel[B].begin(), accel[B].end(), A) == accel[B].end())
					accel[A].push_back(B);
				if (std::find(accel[C].begin(), accel[C].end(), B) == accel[C].end())
					accel[B].push_back(C);
				if (std::find(accel[A].begin(), accel[A].end(), C) == accel[A].end())
					accel[C].push_back(A);
			}
			else
				std::cout << "Warning trying to import face of valence " << aimesh->mFaces[i].mNumIndices << std::endl;
		}

		line_indices.reserve(nb_tri_ind);
		for (GLuint i = 0; i < accel.size(); ++i)
		{
			std::vector<GLuint>& vv = accel[i];
			for (GLuint j = 0; j < vv.size(); ++j)
			{
				line_indices.push_back(i);
				line_indices.push_back(vv[j]);
			}
		}
	}
	else if (mesh->nb_vertices() > 0)
	{
		GLuint nb_tri_ind = aimesh->mNumFaces * 3;
		tri_indices.reserve(nb_tri_ind);

		for (GLuint i = 0; i < nb_tri_ind; ++i)
		{
			tri_indices.push_back(i);
		}

		line_indices.reserve(2 * nb_tri_ind);
		nb_tri_ind /= 3;
		for (GLuint i = 0; i < nb_tri_ind; ++i)
		{
			auto j = 3 * i;
			line_indices.push_back(tri_indices[j++]);
			line_indices.push_back(tri_indices[j]);
			line_indices.push_back(tri_indices[j++]);
			line_indices.push_back(tri_indices[j]);
			line_indices.push_back(tri_indices[j]);
			line_indices.push_back(tri_indices[j - 2]);
		}
		//std::cout << "WARNING GENERATE FAKE ID TRIANGLE INDICES " << std::endl;
		//std::cout << " NB Triangles " << tri_indices.size() / 3 << std::endl;
	}


	// process material
	if (aimesh->mMaterialIndex >= 0)
	{
		auto& mater = mesh->mat_;
		mater->set_name(mesh->name());
		aiColor3D color = aiColor3D(0);
		float shininess = 0.0f;
		float opacity = 1.0f;
		// Read mtl file vertex data
		aimaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
		mater->Ka = GLVec3(color.r, color.g, color.b);
		color = aiColor3D(0);
		aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		mater->Kd = GLVec3(color.r, color.g, color.b);
		color = aiColor3D(1);
		aimaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
		mater->Ks = GLVec3(color.r, color.g, color.b);
		aimaterial->Get(AI_MATKEY_SHININESS, shininess);
		mater->Ns = shininess;
		aimaterial->Get(AI_MATKEY_OPACITY, opacity);
		mater->opacity = opacity;

		load_tex(aimaterial, aiTextureType_DIFFUSE, mater->tex_kd);
		load_tex(aimaterial, aiTextureType_AMBIENT, mater->tex_ka);
		load_tex(aimaterial, aiTextureType_SPECULAR, mater->tex_ks);
		load_tex(aimaterial, aiTextureType_SHININESS, mater->tex_ns);
		//load_tex(aimaterial, aiTextureType_REFLECTION, mater->tex_reflect);
		//		load_tex(aimaterial, aiTextureType_DIFFUSE_ROUGHNESS, mater->tex_rough);	
		load_tex(aimaterial, aiTextureType_OPACITY, mater->tex_opa);
		load_tex(aimaterial, aiTextureType_NORMALS, mater->tex_norm_map);
		load_tex(aimaterial, aiTextureType_HEIGHT, mater->tex_norm_map);
	}
	return mesh;
}


}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
