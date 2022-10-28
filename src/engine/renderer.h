#pragma once
#include <memory>
#include <string>

#include "utils/event_manager.h"

class Texture2D;
DECLARE_DELEGATE_MULTICAST(EventFullscreen, bool);
DECLARE_DELEGATE_MULTICAST(EventResolutionChanged, int, int);

class Material;

struct GLFWwindow;
struct ImGuiContext;

class Renderer final {
public:
    Renderer();
    ~Renderer();

    void initialize() const;

    void submit() const;

    [[nodiscard]] bool        should_close() const;
    [[nodiscard]] GLFWwindow* get_window() const { return main_window; }

    EventResolutionChanged on_resolution_changed;

    void set_icon(const std::string& file_path);

    [[nodiscard]] uint32_t window_height() const { return render_height; }
    [[nodiscard]] uint32_t window_width() const { return render_width; }

private:
    void init_context();

    GLFWwindow*   main_window   = nullptr;
    ImGuiContext* imgui_context = nullptr;
    bool          uniform_explicit_location_support;
    int           render_width  = 0;
    int           render_height = 0;
    void          resize_framebuffer_internal(GLFWwindow*, int x, int y);
};
