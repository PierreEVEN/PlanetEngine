#include <camera.h>
#include <texture2d.h>
#include <GL/gl3w.h>

#include "graphics/material.h"

#include "default_camera_controller.h"
#include "world.h"
#include "engine/asset_manager.h"
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

const double earth_location = 0;// 149597870700;

int main()
{
	Engine::get().get_renderer().set_icon("resources/textures/icon.png");
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
	main_planet->radius = 6000000;
	main_planet->num_lods = 20;
	main_planet->regenerate();
	main_planet->set_local_position({earth_location, 0, -main_planet->radius });
	Engine::get().get_world().get_scene_root().add_child(main_planet);

	const auto secondary_planet = std::make_shared<Planet>(Engine::get().get_world());
	Engine::get().get_world().get_scene_root().add_child(secondary_planet);
	secondary_planet->radius = 1700000;
	secondary_planet->num_lods = 19;
	secondary_planet->regenerate();
	double planet_rotation = 0;

	const auto default_material = Material::create("standard_material");
	default_material->load_from_source("resources/shaders/standard_material.vs",
	                                   "resources/shaders/standard_material.fs");
	const auto cube = std::make_shared<MeshComponent>("cube");
	cube->set_material(default_material);
	cube->set_mesh(primitives::cube());
	Engine::get().get_world().get_scene_root().add_child(cube);

	// Create camera controller
	DefaultCameraController camera_controller(Engine::get().get_world().get_camera());
	camera_controller.teleport_to({ earth_location , 0, 0 });

	while (!Engine::get().get_renderer().should_close())
	{
		Engine::get().get_asset_manager().refresh_dirty_assets();
		{
			STAT_DURATION("Game_loop");
			Engine::get().get_renderer().initialize();

			// Gameplay
			camera_controller.tick(Engine::get().get_world().get_delta_seconds());
			Engine::get().get_world().tick_world();
			planet_rotation += Engine::get().get_world().get_delta_seconds() * 0.02;
			secondary_planet->set_local_position(
				Eigen::Vector3d(std::cos(planet_rotation), 0, std::sin(planet_rotation)) * 300000000 + Eigen::Vector3d(
					earth_location, 0, 0));

			// G_buffers
			{
				STAT_DURATION("Deferred_GBuffers");
				Engine::get().get_renderer().bind_g_buffers();
				Engine::get().get_world().render_world();
			}

			// Deferred combine
			{
				STAT_DURATION("Deferred_Combine");
				Engine::get().get_renderer().bind_deferred_combine();

				g_buffer_combine_material->bind();
				const int color_location = g_buffer_combine_material->binding("color");
				Engine::get().get_renderer().world_color().bind(color_location);
				glUniform1i(color_location, color_location);
				const int normal_location = g_buffer_combine_material->binding("normal");
				Engine::get().get_renderer().world_normal().bind(normal_location);
				glUniform1i(normal_location, normal_location);
				const int depth_location = g_buffer_combine_material->binding("depth");
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
