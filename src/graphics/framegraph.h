#pragma once
#include <memory>
#include <string>
#include <vector>

class RenderPass;

/**
 * \brief A easier way to handle render pass and dependencies between passes.
 */
class FrameGraph {
public:
    static std::shared_ptr<FrameGraph> create(std::string in_name, std::shared_ptr<RenderPass> in_root);

    /**
     * \brief Draw framegraph
     * \param to_back_buffer Set to true to draw final render pass directly on window framebuffers
     */
    void render(bool to_back_buffer, uint32_t in_width, uint32_t in_height);

    /**
     * \brief Set final viewport resolution 
     */
    void resize(uint32_t width, uint32_t height);

    /**
     * \brief This is the last render pass
     */
    [[nodiscard]] const std::shared_ptr<RenderPass>& get_root() const { return root; }

    const std::string name;

    static const std::vector<std::shared_ptr<FrameGraph>>& registry();

    virtual ~FrameGraph();

  private:
    std::shared_ptr<RenderPass> root;

    FrameGraph(std::string in_name, std::shared_ptr<RenderPass> in_root)
        : name(std::move(in_name)), root(std::move(in_root)) {
    }

    bool     resized = false;
    uint32_t width   = 0;
    uint32_t height  = 0;
};
