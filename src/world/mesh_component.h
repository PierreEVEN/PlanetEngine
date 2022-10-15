#pragma once

#include "scene_component.h"

class Material;
class Mesh;

class MeshComponent : public SceneComponent {
public:

	using SceneComponent::SceneComponent;

	virtual ~MeshComponent() = default;

	void set_mesh(const std::shared_ptr<Mesh>& in_mesh) {
		mesh = in_mesh;
	}

	void set_material(const std::shared_ptr<Material>& in_material) {
		material = in_material;
	}

	[[nodiscard]] const std::shared_ptr<Mesh>& get_mesh() const {
		return mesh;
	}


	[[nodiscard]] const std::shared_ptr<Material>& get_material() const {
		return material;
	}

	virtual void render(Camera& camera) override;

	void draw_ui() override;
private:
	std::shared_ptr<Material> material;
	std::shared_ptr<Mesh> mesh;
};