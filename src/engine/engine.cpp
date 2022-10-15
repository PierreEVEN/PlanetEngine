#include "engine.h"

#include <imgui_impl_glfw.h>
#include <iostream>
#include <memory>
#include <GLFW/glfw3.h>

#include "asset_manager.h"
#include "renderer.h"
#include "world/world.h"

static std::unique_ptr<Engine> engine;

Engine& Engine::get()
{
	if (!engine)
	{
		engine = std::unique_ptr<Engine>(new Engine());
		engine->init();
	}
	return *engine;
}

Engine::~Engine()
{
	world = nullptr;
	renderer = nullptr;
	asset_manager = nullptr;
}

void Engine::init()
{
	asset_manager = std::shared_ptr<AssetManager>(new AssetManager());
	renderer = std::make_shared<Renderer>();
	world = std::make_shared<World>();

	glfwSetKeyCallback(get().get_renderer().get_window(),
	                   [](GLFWwindow* window, int key, int scan_code, int action, int mode)
	                   {
		                   ImGui_ImplGlfw_KeyCallback(window, key, scan_code, action, mode);
		                   get().on_key_down.execute(window, key, scan_code, action, mode);

		                   if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
			                   get().renderer->switch_fullscreen();

						   if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
							   get().renderer->wireframe = !get().renderer->wireframe;
	                   }
	);

	glfwSetCursorPosCallback(get().get_renderer().get_window(), [](GLFWwindow* window, double xpos, double ypos)
	                         {
		                         ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
		                         get().on_mouse_moved.execute(window, xpos, ypos);
	                         }
	);

	glfwSetFramebufferSizeCallback(get().get_renderer().get_window(),
	                               [](GLFWwindow* window, int width, int height)
	                               {
		                               get().on_framebuffer_resized.execute(window, width, height);
	                               }
	);

	glfwSetScrollCallback(get().get_renderer().get_window(),
	                      [](GLFWwindow* window, double xoffset, double yoffset)
	                      {
		                      ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
		                      get().on_mouse_scroll.execute(window, xoffset, yoffset);
	                      }
	);
}
