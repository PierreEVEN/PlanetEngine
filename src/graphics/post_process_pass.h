#pragma once
#include <functional>
#include <memory>
#include <string>

#include "render_pass.h"

class Renderer;

namespace EZCOGL {
class TextureInterface;
class FBO;
}

class Material;

class PostProcessPass : public RenderPass {
public:
    static std::shared_ptr<PostProcessPass> create(const std::string& name, Renderer& parent) {
        return std::shared_ptr<PostProcessPass>(new PostProcessPass(name, parent));
    }

    static std::shared_ptr<PostProcessPass> create(const std::string& name, std::shared_ptr<PostProcessPass> previous_pass) {
        return std::shared_ptr<PostProcessPass>(new PostProcessPass(name, previous_pass));
    }

    virtual ~PostProcessPass();

    void init(const std::string& fragment_shader);

    const std::string name;

    [[nodiscard]] const std::shared_ptr<EZCOGL::TextureInterface>& result() const { return texture; }
    [[nodiscard]] const std::shared_ptr<Material>&                 material() const { return pass_material; }

    void bind(bool to_back_buffer = false) const;
    void draw() const;

    void on_resolution_changed(const std::function<void(int&, int&)>& callback) {
        resolution_changed_callback = callback;
    }

    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;

private:
    PostProcessPass(std::string in_name, Renderer& in_parent);
    PostProcessPass(std::string in_name, const std::shared_ptr<PostProcessPass>& previous_pass);
    PostProcessPass(std::string in_name, Renderer& in_parent, const std::shared_ptr<EZCOGL::TextureInterface>& in_previous_texture);
    void resolution_changed(int x, int y);

    std::shared_ptr<Material>                 pass_material;
    std::shared_ptr<EZCOGL::TextureInterface> texture;
    std::shared_ptr<EZCOGL::FBO>              framebuffer;
    std::function<void(int&, int&)>           resolution_changed_callback = nullptr;
    Renderer&                                 parent_renderer;
    std::shared_ptr<EZCOGL::TextureInterface> previous_texture;
};


class PostProcessPassV2 : public RenderPass {
public:
    static std::shared_ptr<PostProcessPassV2> create(const std::string& name, uint32_t width, uint32_t height, const std::string& fragment_shader) {
      return std::shared_ptr<PostProcessPassV2>(new PostProcessPassV2(name, width, height, fragment_shader));
    }

    virtual ~PostProcessPassV2() = default;
    
    [[nodiscard]] const std::shared_ptr<Material>& material() const { return pass_material; }


    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;
    void render(bool to_back_buffer) override;
private:
    PostProcessPassV2(std::string in_name, uint32_t width, uint32_t height, const std::string& fragment_shader);
    void resolution_changed(int x, int y);

    std::shared_ptr<Material>       pass_material;
};
