#pragma once
#include <memory>
#include "utils/event_manager.h"


class World;
class Renderer;
class AssetManager;

struct GLFWwindow;
DECLARE_DELEGATE_MULTICAST(EventKeyboard, GLFWwindow*, int, int, int, int);
DECLARE_DELEGATE_MULTICAST(EventMousePos, GLFWwindow*, double, double);
DECLARE_DELEGATE_MULTICAST(EventResizeFramebuffer, GLFWwindow*, int, int);
DECLARE_DELEGATE_MULTICAST(EventScroll, GLFWwindow*, double, double);


class Engine final
{
public:
	static Engine& get();
	~Engine();

	[[nodiscard]] AssetManager& get_asset_manager() const
	{
		return *asset_manager;
	}

	[[nodiscard]] Renderer& get_renderer() const
	{
		return *renderer;
	}

	[[nodiscard]] World& get_world() const
	{
		return *world;
	}

	EventKeyboard on_key_down;
	EventMousePos on_mouse_moved;
	EventResizeFramebuffer on_window_resized;
	EventScroll on_mouse_scroll;

	void init();

private:
	Engine() = default;
	std::shared_ptr<World> world;
	std::shared_ptr<Renderer> renderer;
	std::shared_ptr<AssetManager> asset_manager;
};
