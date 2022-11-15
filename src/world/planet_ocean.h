#pragma once
#include "scene_component.h"

class Planet;
class Material;
class Mesh;

class PlanetOcean : public SceneComponent {
public:
    PlanetOcean(const std::shared_ptr<Planet>& owning_planet);
protected:
    void render(Camera& camera, const DrawGroup& draw_group, const std::shared_ptr<RenderPass>& render_pass) override;
private:
    std::shared_ptr<Mesh>     grid_mesh;
    std::shared_ptr<Material> ocean_material;
};
