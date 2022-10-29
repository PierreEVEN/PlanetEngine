#pragma once
#include <functional>
#include <memory>
#include <string>

#include "render_pass.h"
#include "texture_image.h"

class Renderer;

class Material;
DECLARE_DELEGATE_MULTICAST(EventBindMaterial, std::shared_ptr<Material>);

class PostProcessPass : public RenderPass {
public:
    static std::shared_ptr<PostProcessPass>
    create(const std::string& name, uint32_t width, uint32_t height, const std::string& fragment_shader,
         TextureCreateInfos create_infos = {.wrapping = TextureWrapping::ClampToEdge, .filtering_mag = TextureMagFilter::Linear, .filtering_min = TextureMinFilter::Linear}) {
        return std::shared_ptr<PostProcessPass>(new PostProcessPass(name, width, height, fragment_shader, create_infos));
    }

    virtual ~PostProcessPass() = default;

    [[nodiscard]] const std::shared_ptr<Material>& material() const { return pass_material; }

    EventBindMaterial on_bind_material;

    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;
    void              render(bool to_back_buffer) override;
private:
    PostProcessPass(std::string in_name, uint32_t width, uint32_t height, const std::string& fragment_shader, TextureCreateInfos create_infos);
    void resolution_changed(int x, int y);

    std::shared_ptr<Material> pass_material;
};
