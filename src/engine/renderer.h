#pragma once
#include <memory>

#include "utils/event_manager.h"

DECLARE_DELEGATE_MULTICAST(EventFullscreen, bool);

class Material;

namespace EZCOGL
{
	class FBO;
	class Texture2D;
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
	void bind_deferred_combine() const;

	void submit() const;

	[[nodiscard]] bool should_close() const;
	[[nodiscard]] GLFWwindow* get_window() const { return main_window; }

	[[nodiscard]] EZCOGL::Texture2D& world_color() const { return *g_buffer_color; }
	[[nodiscard]] EZCOGL::Texture2D& world_normal() const { return *g_buffer_normal; }
	[[nodiscard]] EZCOGL::Texture2D& world_depth() const { return *g_buffer_depth; }
	[[nodiscard]] EZCOGL::FBO_DepthTexture& framebuffer() const { return *g_buffer; }

	[[nodiscard]] EZCOGL::Texture2D& get_resolve_texture() const { return *resolve_texture; }

	void switch_fullscreen();
	[[nodiscard]] bool is_fullscreen() const { return fullscreen; }

	void resize_framebuffer_internal(GLFWwindow*, int x, int y);

	bool wireframe = false;
	EventFullscreen on_fullscreen;
private:
	void init_context();

	std::shared_ptr<EZCOGL::FBO_DepthTexture> g_buffer;
	std::shared_ptr<EZCOGL::Texture2D> g_buffer_color;
	std::shared_ptr<EZCOGL::Texture2D> g_buffer_normal;
	std::shared_ptr<EZCOGL::Texture2D> g_buffer_depth;

	std::shared_ptr<EZCOGL::FBO> resolve_framebuffer;
	std::shared_ptr<EZCOGL::Texture2D> resolve_texture;

	bool fullscreen = false;
	GLFWwindow* main_window = nullptr;
	ImGuiContext* imgui_context = nullptr;
	bool uniform_explicit_location_support;
};
