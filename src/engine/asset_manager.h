#pragma once
#include <vector>

class ComputeShader;
class TextureBase;
class Mesh;
class Material;

class AssetManager
{
	friend class Engine;
	friend class Mesh;
	friend class Material;
	friend class TextureBase;
	friend class ComputeShader;
public:
	[[nodiscard]] const std::vector<Mesh*>& get_meshes() const { return meshes; }
	[[nodiscard]] const std::vector<Material*>& get_materials() const { return materials; }
	[[nodiscard]] const std::vector<ComputeShader*>& get_computes() const { return compute_shaders; }
	[[nodiscard]] const std::vector<TextureBase*>& get_textures() const { return textures; }

	void refresh_dirty_assets() const;
private:
	AssetManager() = default;
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	std::vector<ComputeShader*> compute_shaders;
	std::vector<TextureBase*> textures;
};