#include "graphics/camera.h"
#include <imgui.h>
#include <GL/gl3w.h>

#include "graphics/material.h"

#include "default_camera_controller.h"
#include "world/world.h"
#include "engine/asset_manager.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/easycppogl_texture.h"
#include "world/planet.h"
#include "graphics/primitives.h"
#include "graphics/texture_cube.h"
#include "ui/asset_manager_ui.h"
#include "ui/graphic_debugger.h"
#include "ui/session_frontend.h"
#include "ui/ui.h"
#include "ui/viewport.h"
#include "ui/world_outliner.h"
#include "utils/profiler.h"
#include "world/cubemap_component.h"
#include "world/mesh_component.h"

const double earth_location = 0; // 149597870700;

int main()
{
	Engine::get().get_renderer().set_icon("resources/textures/icon.png");
	const auto g_buffer_combine_material = Material::create("g_buffer combine");
	g_buffer_combine_material->load_from_source("resources/shaders/gbuffer_combine.vs",
	                                            "resources/shaders/gbuffer_combine.fs");

	ImGuiWindow::create_window<GraphicDebugger>();
	ImGuiWindow::create_window<MaterialManagerUi>();
	ImGuiWindow::create_window<TextureManagerUi>();
	ImGuiWindow::create_window<Viewport>();
	ImGuiWindow::create_window<SessionFrontend>();
	ImGuiWindow::create_window<WorldOutliner>(&Engine::get().get_world());

	// Create planet
	const auto earth = std::make_shared<Planet>("earth");
	earth->radius = 6000000;
	earth->num_lods = 19;
	earth->cell_count = 30;
	earth->regenerate();
	earth->set_local_position({earth_location, 0, 0});
	Engine::get().get_world().get_scene_root().add_child(earth);

	const auto moon = std::make_shared<Planet>("moon");
	Engine::get().get_world().get_scene_root().add_child(moon);
	moon->radius = 1700000;
	moon->num_lods = 18;
	moon->cell_count = 30;
	moon->regenerate();
	double moon_orbit = 0;
	double moon_rotation = 0;
	double hearth_rotation = 0;

	earth->add_child(Engine::get().get_world().get_camera());

	const auto default_material = Material::create("standard_material");
	default_material->load_from_source("resources/shaders/standard_material.vs",
	                                   "resources/shaders/standard_material.fs");
	const auto cube = std::make_shared<MeshComponent>("cube");
	cube->set_material(default_material);
	cube->set_mesh(primitives::cube());
	cube->set_local_position({0, 0, earth->radius});
	Engine::get().get_world().get_scene_root().add_child(cube);
	earth->add_child(cube);

	// Create camera controller
	DefaultCameraController camera_controller(Engine::get().get_world().get_camera());
	camera_controller.teleport_to({-10, 0, earth->radius + 20});


	const auto cubemap = TextureCube::create("cube map");
	cubemap->from_file("resources/textures/skybox/py.png", "resources/textures/skybox/ny.png",
	                   "resources/textures/skybox/px.png", "resources/textures/skybox/nx.png",
	                   "resources/textures/skybox/pz.png","resources/textures/skybox/nz.png");

	while (!Engine::get().get_renderer().should_close())
	{
		Engine::get().get_asset_manager().refresh_dirty_assets();
		{
			STAT_DURATION("Game_loop");
			Engine::get().get_renderer().initialize();

			// Gameplay
			camera_controller.tick(Engine::get().get_world().get_delta_seconds());
			Engine::get().get_world().tick_world();
			moon_orbit += Engine::get().get_world().get_delta_seconds() * 0.02;
			moon_rotation += Engine::get().get_world().get_delta_seconds() * 0.2;
			hearth_rotation += Engine::get().get_world().get_delta_seconds() * 0.05;

			moon->set_local_position(
				Eigen::Vector3d(std::cos(moon_orbit), 0, std::sin(moon_orbit)) * 30000000 + Eigen::Vector3d(
					earth_location, 0, 0));

			moon->set_local_rotation(
				Eigen::Quaterniond(Eigen::AngleAxisd(moon_rotation, Eigen::Vector3d::UnitY())));

			earth->set_local_rotation(
				Eigen::Quaterniond(Eigen::AngleAxisd(hearth_rotation, Eigen::Vector3d::UnitY())));

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
				glUniform1f(g_buffer_combine_material->binding("z_near"),
				            static_cast<float>(Engine::get().get_world().get_camera()->z_near()));
				g_buffer_combine_material->bind_texture(
					dynamic_pointer_cast<EasyCppOglTexture>(Engine::get().get_renderer().world_color()),
					"GBUFFER_color");
				g_buffer_combine_material->bind_texture(
					dynamic_pointer_cast<EasyCppOglTexture>(Engine::get().get_renderer().world_normal()),
					"GBUFFER_normal");
				g_buffer_combine_material->bind_texture(
					dynamic_pointer_cast<EasyCppOglTexture>(Engine::get().get_renderer().world_depth()),
					"GBUFFER_depth");
				g_buffer_combine_material->bind_texture(cubemap, "WORLD_Cubemap");
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			// UI
			ui::draw();

			Engine::get().get_renderer().submit();
		}
		Profiler::get().new_frame();
	}
}
