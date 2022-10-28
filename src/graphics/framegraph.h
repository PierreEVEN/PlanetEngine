#pragma once
#include <memory>
#include <string>
#include <vector>

class RenderPass;

class FrameGraph {
public:
    static std::shared_ptr<FrameGraph> create(std::string in_name, std::shared_ptr<RenderPass> in_root) {
        return std::shared_ptr<FrameGraph>(new FrameGraph(std::move(in_name), std::move(in_root)));
    }

    void render(bool to_back_buffer, uint32_t in_width, uint32_t in_height);
    void render(bool to_back_buffer);

    const std::string name;

    void resize(uint32_t width, uint32_t height);

    [[nodiscard]] const std::shared_ptr<RenderPass>& get_root() const { return root; }

private:
    std::shared_ptr<RenderPass> root;

    FrameGraph(std::string in_name, std::shared_ptr<RenderPass> in_root)
        : name(std::move(in_name)), root(std::move(in_root)) {
    }

    bool     resized = false;
    uint32_t width   = 0;
    uint32_t height  = 0;
};
