#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <utils/event_manager.h>

class Texture2D;
enum class ImageFormat;
struct TextureCreateInfos;
class Material;

DECLARE_DELEGATE_MULTICAST(EventDraw);

class Attachment {
public:
    Attachment(std::string in_name, ImageFormat in_format, int in_binding_index)
        : name(std::move(in_name)),
          binding_index(in_binding_index),
          format(in_format) {
    }

    virtual void init(uint32_t width, uint32_t height) = 0;

    [[nodiscard]] virtual std::shared_ptr<Texture2D> get_render_target() const = 0;

    const std::string name;
    const int         binding_index;
    const ImageFormat format;
};

class TextureAttachment : public Attachment {
public:
    TextureAttachment(std::string framebuffer_name, std::string in_name, const TextureCreateInfos& create_infos, ImageFormat in_format, int binding_index, uint32_t framebuffer);
    void                                     init(uint32_t width, uint32_t height) override;
    [[nodiscard]] std::shared_ptr<Texture2D> get_render_target() const override { return render_target; }

private:
    std::shared_ptr<Texture2D> render_target;
};

class RenderBufferAttachment : public Attachment {
public:
    RenderBufferAttachment(std::string in_name, ImageFormat in_format, int binding_index, uint32_t framebuffer);
    virtual                                  ~RenderBufferAttachment();
    void                                     init(uint32_t width, uint32_t height) override;
    [[nodiscard]] std::shared_ptr<Texture2D> get_render_target() const override { return nullptr; }

private:
    uint32_t rbo;
};

class RenderPass {
public:
    static std::shared_ptr<RenderPass> create(std::string name, uint32_t width, uint32_t height) {
        return std::shared_ptr<RenderPass>(new RenderPass(name, width, height));
    }

    virtual ~RenderPass();

    virtual void render(bool to_back_buffer);
    virtual void reset();
    void         resize(uint32_t width, uint32_t height);

    void add_attachment(const std::string& attachment_name, ImageFormat image_format, const TextureCreateInfos& create_infos, bool write_only = false);

    void link_dependency(const std::shared_ptr<RenderPass>& dependency) {
        dependencies.emplace_back(dependency);
    }

    void link_dependency(const std::shared_ptr<RenderPass>& dependency, const std::vector<std::string>& color_bind_points, const std::string& depth_bind_point = "") {
        if (dependency->get_depth_attachment() && !depth_bind_point.empty() || color_bind_points.size() == dependency->get_color_attachments().size())
            bind_points[dependencies.size()] = {color_bind_points, depth_bind_point};
        dependencies.emplace_back(dependency);
    }

    void on_compute_resolution(const std::function<void(uint32_t&, uint32_t&)>& callback) { compute_resolution = callback; }

    [[nodiscard]] uint32_t                                        get_width() const { return width; }
    [[nodiscard]] uint32_t                                        get_height() const { return height; }
    [[nodiscard]] const std::vector<std::unique_ptr<Attachment>>& get_color_attachments() const { return color_attachments; };
    [[nodiscard]] const std::unique_ptr<Attachment>&              get_depth_attachment() const { return depth_attachment; };

    EventDraw         on_draw;
    const std::string name;

protected:
    bool pre_render();
  void bind(bool back_buffer);

    RenderPass(std::string name, uint32_t width, uint32_t height);
    std::vector<std::shared_ptr<RenderPass>>                                     dependencies;
    uint32_t                                                                     framebuffer_id = 0;
    std::unordered_map<size_t, std::pair<std::vector<std::string>, std::string>> bind_points;

private:
    void     init_attachments();
    bool     is_dirty = true;
    bool     complete = false;
    uint32_t width    = 0;
    uint32_t height   = 0;

    std::vector<std::unique_ptr<Attachment>>  color_attachments;
    std::unique_ptr<Attachment>               depth_attachment;
    std::function<void(uint32_t&, uint32_t&)> compute_resolution = nullptr;
};
