#include "graphics/renderer.h"

#include <camera.h>
#include <fbo.h>
#include <mesh.h>
#include <shader_program.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "graphics/material.h"

#include "default_camera_controller.h"
#include "world.h"
#include "graphics/mesh.h"
#include "graphics/primitives.h"
#include "world/mesh_component.h"

std::unique_ptr<DefaultCameraController> camera_controller;
std::shared_ptr<Camera> world_camera;
std::shared_ptr<EZCOGL::FBO> g_buffer;

int main()
{
	const Renderer renderer;
	auto world = World();
	camera_controller = std::make_unique<DefaultCameraController>(world.get_camera());
	world_camera = world.get_camera();

	glfwSetKeyCallback(renderer.get_window(), [](GLFWwindow* window, int key, int scan_code, int action, int mode)
	                   {
		                   camera_controller->process_key(key, scan_code, action, mode);
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

	std::vector<std::shared_ptr<EZCOGL::Texture2D>> textures;
	// GBuffer color
	const auto g_buffer_color = EZCOGL::Texture2D::create();
	g_buffer_color->alloc(800, 600, GL_RGB8, nullptr);
	textures.push_back(g_buffer_color);
	// GBuffer normal
	const auto g_buffer_normal = EZCOGL::Texture2D::create();
	g_buffer_normal->alloc(800, 600, GL_RGB8, nullptr);
	textures.push_back(g_buffer_normal);
	g_buffer = EZCOGL::FBO::create(textures);

	const auto g_buffer_combine = Material::create("g_buffer_combine");
	g_buffer_combine->load_from_source("resources/shaders/gbuffer_combine.vs", "resources/shaders/gbuffer_combine.fs");

	const auto standard_material = Material::create("standard_material");
	standard_material->load_from_source("resources/shaders/standard_material.vs",
	                                    "resources/shaders/standard_material.fs");

	const auto cube_mesh = primitives::cube();
	const auto mesh_comp = std::make_shared<MeshComponent>();
	mesh_comp->set_mesh(cube_mesh);
	mesh_comp->set_material(standard_material);
	mesh_comp->set_local_position({8, 0, 0});
	world.get_scene_root().add_child(mesh_comp);


	const auto mesh_comp_2 = std::make_shared<MeshComponent>();
	mesh_comp_2->set_mesh(cube_mesh);
	mesh_comp_2->set_material(standard_material);
	mesh_comp_2->set_local_position({ 2, 0, 0 });
	mesh_comp->add_child(mesh_comp_2);


	float x = 0, y = 0, z = 0;
	float px = 8, py = 0, pz = 0;

	while (!renderer.should_close())
	{
		renderer.begin();

		world.tick_world();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);

		ImGui::Separator();
		ImGui::SliderFloat("rot_x", &x, -M_PI, M_PI);
		ImGui::SliderFloat("rot_y", &y, -M_PI, M_PI);
		ImGui::SliderFloat("rot_z", &z, -M_PI, M_PI);
		ImGui::SliderFloat("pos_x", &px, -10, 10);
		ImGui::SliderFloat("pos_y", &py, -10, 10);
		ImGui::SliderFloat("pos_z", &pz, -10, 10);
		ImGui::Separator();
		mesh_comp->set_local_rotation(Eigen::Quaterniond(Eigen::AngleAxisd(z, Eigen::Vector3d::UnitZ()) * Eigen::AngleAxisd(y, Eigen::Vector3d::UnitY()) * Eigen::AngleAxisd(x, Eigen::Vector3d::UnitX())));
		mesh_comp->set_local_position({ px, py, pz });

		/**
		 * GBUFFERS
		 */
		g_buffer->bind();
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

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
		g_buffer_color->bind();

		EZCOGL::VAO::none()->bind();
		glDrawArrays(GL_TRIANGLES, 0, 3);

		/*
		 * DEMO WINDOW
		 */
		if (ImGui::Begin("toto"))
			ImGui::Text("yay");
		ImGui::End();

		renderer.end();
	}
}
