#pragma once
#include <vector>

class TextureImage;
class Mesh;
class Material;

class AssetManager
{
	friend class Engine;
	friend class Mesh;
	friend class Material;
	friend class TextureImage;
public:
	[[nodiscard]] const std::vector<Mesh*>& get_meshes() const { return meshes; }
	[[nodiscard]] const std::vector<Material*>& get_materials() const { return materials; }
	[[nodiscard]] const std::vector<TextureImage*>& get_textures() const { return textures; }
private:
	AssetManager() = default;
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	std::vector<TextureImage*> textures;
};
