
#include "primitives.h"
#include "graphics/mesh.h"

namespace primitives {

	std::shared_ptr<Mesh> cube(float size)
	{
		auto mesh = Mesh::create("cube");
		
		float V = size;
		float v = -size;
		
		mesh->set_positions(std::vector<Eigen::Vector3f>{
				{v,v,v}, {V,v,v}, {V,V,v}, {v,V,v},
				{v,v,V}, {V,v,V}, {V,V,V}, {v,V,V},
				{v,v,V}, {v,v,v}, {v,V,v}, {v,V,V},
				{V,v,V}, {V,v,v}, {V,V,v}, {V,V,V},
				{v,v,V}, {V,v,V}, {V,v,v}, {v,v,v},
				{v,V,V}, {V,V,V}, {V,V,v}, {v,V,v} }, 0, true);

		mesh->set_colors(std::vector<Eigen::Vector3f>{
			{1, 1, 0}, { 1,1,0 }, { 1,1,0 }, { 1,1,0 },
			{ 0,0,1 }, { 0,0,1 }, { 0,0,1 }, { 0,0,1 },
			{ 0,1,1 }, { 0,1,1 }, { 0,1,1 }, { 0,1,1 },
			{ 1,0,0 }, { 1,0,0 }, { 1,0,0 }, { 1,0,0 },
			{ 1,0,1 }, { 1,0,1 }, { 1,0,1 }, { 1,0,1 },
			{ 0,1,0 }, { 0,1,0 }, { 0,1,0 }, { 0,1,0 } }, 3, true);

		mesh->set_normals(std::vector<Eigen::Vector3f>{
			{0, 0, -1}, { 0,0,-1 }, { 0,0,-1 }, { 0,0,-1 },
			{ 0,0,1 }, { 0,0,1 }, { 0,0,1 }, { 0,0,1 },
			{ -1,0,0 }, { -1,0,0 }, { -1,0,0 }, { -1,0,0 },
			{ 1,0,0 }, { 1,0,0 }, { 1,0,0 }, { 1,0,0 },
			{ 0,-1,0 }, { 0,-1,0 }, { 0,-1,0 }, { 0,-1,0 },
			{ 0,1,0 }, { 0,1,0 }, { 0,1,0 }, { 0,1,0 } }, 1, true);

		mesh->set_texture_coordinates(std::vector<Eigen::Vector2f>{
			{0, 0}, { 1,0 }, { 1,1 }, { 0,1 },
			{ 0,0 }, { 1,0 }, { 1,1 }, { 0,1 },
			{ 0,0 }, { 1,0 }, { 1,1 }, { 0,1 },
			{ 0,0 }, { 1,0 }, { 1,1 }, { 0,1 },
			{ 0,0 }, { 1,0 }, { 1,1 }, { 0,1 },
			{ 0,0 }, { 1,0 }, { 1,1 }, { 0,1 } }, 2, true);

		mesh->set_indices(std::vector<uint32_t>{ 0,3,2,0,2,1, 4,5,6,4,6,7, 8,11,10,8,10,9, 12,13,14,12,14,15, 16,19,18,16,18,17, 20,21,22,20,22,23 }, true);

		mesh->rebuild_mesh_data();

		return mesh;
	}
}