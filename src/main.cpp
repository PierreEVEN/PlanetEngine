#include "graphics/renderer.h"

#include <camera.h>
#include <fbo.h>
#include <mesh.h>
#include <shader_program.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

#include "graphics/material.h"

#include "default_camera_controller.h"
#include "world.h"
#include "geometry/Planet.h"
#include "graphics/primitives.h"
#include "ui/ui.h"
#include "world/mesh_component.h"

std::unique_ptr<DefaultCameraController> camera_controller;
std::shared_ptr<Camera> world_camera;
std::shared_ptr<EZCOGL::FBO_DepthTexture> g_buffer;

int main()
{
	const Renderer renderer;
	auto world = World();
	camera_controller = std::make_unique<DefaultCameraController>(world.get_camera());
	world_camera = world.get_camera();

	glfwSetKeyCallback(renderer.get_window(), [](GLFWwindow* window, int key, int scan_code, int action, int mode)
	                   {
		                   camera_controller->process_key(window, key, scan_code, action, mode);
		                   ImGui_ImplGlfw_KeyCallback(window, key, scan_code, action, mode);
	                   }
	);
	
	glfwSetCursorPosCallback(renderer.get_window(), [](GLFWwindow* window, double xpos, double ypos)
	                         {
		                         camera_controller->process_mouse_input(xpos, ypos);
		                         ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
	                         }
	);

	glfwSetFramebufferSizeCallback(renderer.get_window(), [](GLFWwindow* window, int width, int height)
	                               {
		                               world_camera->viewport_res() = {width, height};
		                               g_buffer->resize(width, height);
	                               }
	);

	glfwSetScrollCallback(renderer.get_window(), [](GLFWwindow* window, double xoffset, double yoffset)
	                      {
		                      camera_controller->process_mouse_wheel(xoffset, yoffset);
		                      ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	                      }
	);

	std::vector<std::shared_ptr<EZCOGL::Texture2D>> textures;
	// GBuffer color
	const auto g_buffer_color = EZCOGL::Texture2D::create();
	g_buffer_color->alloc(800, 600, GL_RGB8, nullptr);
	textures.push_back(g_buffer_color);
	// GBuffer normal
	const auto g_buffer_normal = EZCOGL::Texture2D::create();
	g_buffer_normal->alloc(800, 600, GL_RGB8, nullptr);
	textures.push_back(g_buffer_normal);
	// GBuffer depth
	const auto g_buffer_depth = EZCOGL::Texture2D::create();
	g_buffer_depth->alloc(800, 600, GL_DEPTH_COMPONENT24, nullptr);
	//textures.push_back(g_buffer_depth);
	g_buffer = EZCOGL::FBO_DepthTexture::create(textures, g_buffer_depth);

	const auto g_buffer_combine = Material::create("g_buffer_combine");
	g_buffer_combine->load_from_source("resources/shaders/gbuffer_combine.vs", "resources/shaders/gbuffer_combine.fs");

	const auto standard_material = Material::create("standard_material");
	standard_material->load_from_source("resources/shaders/standard_material.vs",
	                                    "resources/shaders/standard_material.fs");

	const auto cube_mesh = primitives::cube();
	const auto mesh_comp = std::make_shared<MeshComponent>();
	mesh_comp->set_mesh(cube_mesh);
	mesh_comp->set_material(standard_material);
	world.get_scene_root().add_child(mesh_comp);

	const auto main_planet = std::make_shared<Planet>(world);
	world.get_scene_root().add_child(main_planet);

	Eigen::Matrix4f test = Eigen::Matrix4f::Identity();

	while (!renderer.should_close())
	{
		renderer.begin();

		camera_controller->tick(world.get_delta_seconds());
		world.tick_world();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		/**
		 * GBUFFERS
		 */
		g_buffer->bind();
		glClearColor(0, 0, 0, 0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// Render world
		world.render_world();

		/**
		 * GBUFFERS COMBINE
		 */
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		glClearColor(0.0, 1.0, 0.0, 0.0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		g_buffer_combine->use();

		const int color_location = glGetUniformLocation(g_buffer_combine->program_id(), "color");
		const int normal_location = glGetUniformLocation(g_buffer_combine->program_id(), "normal");
		const int depth_location = glGetUniformLocation(g_buffer_combine->program_id(), "depth");

		glUniform1i(color_location, color_location);
		g_buffer_color->bind(color_location);

		glUniform1i(normal_location, normal_location);
		g_buffer_normal->bind(normal_location);

		glUniform1i(depth_location, depth_location);
		g_buffer_depth->bind(depth_location);

		EZCOGL::VAO::none()->bind();
		glDrawArrays(GL_TRIANGLES, 0, 3);

		ui::draw(renderer, world, g_buffer);
		
		renderer.end();
	}
}
