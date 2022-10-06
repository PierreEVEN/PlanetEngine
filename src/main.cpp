#include <camera.h>
#include <texture2d.h>
#include <GL/gl3w.h>

#include "graphics/material.h"

#include "default_camera_controller.h"
#include "world.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "geometry/planet.h"
#include "graphics/primitives.h"
#include "ui/asset_manager_ui.h"
#include "ui/graphic_debugger.h"
#include "ui/session_frontend.h"
#include "ui/ui.h"
#include "ui/viewport.h"
#include "ui/world_outliner.h"
#include "utils/profiler.h"
#include "world/mesh_component.h"

int main()
{
	const auto g_buffer_combine_material = Material::create("g_buffer combine");
	g_buffer_combine_material->load_from_source("resources/shaders/gbuffer_combine.vs",
	                                            "resources/shaders/gbuffer_combine.fs");

	ImGuiWindow::create_window<GraphicDebugger>();
	ImGuiWindow::create_window<MaterialManagerUi>();
	ImGuiWindow::create_window<TextureManagerUi>();
	ImGuiWindow::create_window<MeshManagerUi>();
	ImGuiWindow::create_window<Viewport>();
	ImGuiWindow::create_window<SessionFrontend>();
	ImGuiWindow::create_window<WorldOutliner>(&Engine::get().get_world());

	// Create planet
	const auto main_planet = std::make_shared<Planet>(Engine::get().get_world());
	Engine::get().get_world().get_scene_root().add_child(main_planet);

	const auto default_material = Material::create("standard_material");
	default_material->load_from_source("resources/shaders/standard_material.vs",
		"resources/shaders/standard_material.fs");
	const auto cube = std::make_shared<MeshComponent>("cube");
	cube->set_material(default_material);
	cube->set_mesh(primitives::cube());
	Engine::get().get_world().get_scene_root().add_child(cube);

	// Create camera controller
	DefaultCameraController camera_controller(Engine::get().get_world().get_camera());
	Engine::get().get_world().get_camera()->set_local_position({0, 0, 10});

	while (!Engine::get().get_renderer().should_close())
	{
		{
			STAT_DURATION(Game_loop);
			Engine::get().get_renderer().initialize();

			// Gameplay
			camera_controller.tick(Engine::get().get_world().get_delta_seconds());
			Engine::get().get_world().tick_world();

			// G_buffers
			{
				STAT_DURATION(Deferred_GBuffers);
				Engine::get().get_renderer().bind_g_buffers();
				Engine::get().get_world().render_world();
			}

			// Deferred combine
			{
				STAT_DURATION(Deferred_Combine);
				Engine::get().get_renderer().bind_deferred_combine();

				g_buffer_combine_material->use();
				const int position_location = glGetUniformLocation(g_buffer_combine_material->program_id(), "position");
				Engine::get().get_renderer().world_position().bind(position_location);
				glUniform1i(position_location, position_location);
				const int color_location = glGetUniformLocation(g_buffer_combine_material->program_id(), "color");
				Engine::get().get_renderer().world_color().bind(color_location);
				glUniform1i(color_location, color_location);
				const int normal_location = glGetUniformLocation(g_buffer_combine_material->program_id(), "normal");
				Engine::get().get_renderer().world_normal().bind(normal_location);
				glUniform1i(normal_location, normal_location);
				const int depth_location = glGetUniformLocation(g_buffer_combine_material->program_id(), "depth");
				Engine::get().get_renderer().world_depth().bind(depth_location);
				glUniform1i(depth_location, depth_location);
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			// UI
			ui::draw();

			Engine::get().get_renderer().submit();
		}
		Profiler::get().new_frame();
	}
}
