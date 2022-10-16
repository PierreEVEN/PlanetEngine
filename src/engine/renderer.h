#pragma once
#include <memory>
#include <string>

#include "utils/event_manager.h"

class Texture2D;
DECLARE_DELEGATE_MULTICAST(EventFullscreen, bool);
DECLARE_DELEGATE_MULTICAST(EventResolutionChanged, int, int);

class Material;

namespace EZCOGL
{
	class TextureInterface;
	class FBO;
	class FBO_DepthTexture;
}

struct GLFWwindow;
struct ImGuiContext;

class Renderer final
{
public:
	Renderer();
	~Renderer();

	void initialize() const;
	void bind_g_buffers() const;

	void submit() const;

	[[nodiscard]] bool should_close() const;
	[[nodiscard]] GLFWwindow* get_window() const { return main_window; }

	[[nodiscard]] std::shared_ptr<EZCOGL::TextureInterface> world_color() const { return g_buffer_color; }
	[[nodiscard]] std::shared_ptr<EZCOGL::TextureInterface> world_normal() const { return g_buffer_normal; }
	[[nodiscard]] std::shared_ptr<EZCOGL::TextureInterface> world_depth() const { return g_buffer_depth; }
	[[nodiscard]] std::shared_ptr<EZCOGL::FBO_DepthTexture> framebuffer() const { return g_buffer; }

	void switch_fullscreen();
	[[nodiscard]] bool is_fullscreen() const { return fullscreen; }

	void resize_framebuffer_internal(GLFWwindow*, int x, int y);

	bool wireframe = false;
	EventFullscreen on_fullscreen;
	EventResolutionChanged on_resolution_changed;

	void set_icon(const std::string& file_path);

private:
	void init_context();

	std::shared_ptr<EZCOGL::FBO_DepthTexture> g_buffer;
	std::shared_ptr<EZCOGL::TextureInterface> g_buffer_color;
	std::shared_ptr<EZCOGL::TextureInterface> g_buffer_normal;
	std::shared_ptr<EZCOGL::TextureInterface> g_buffer_depth;

	bool fullscreen = false;
	GLFWwindow* main_window = nullptr;
	ImGuiContext* imgui_context = nullptr;
	bool uniform_explicit_location_support;

};
