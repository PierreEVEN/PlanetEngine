#include "graphics/camera.h"
#include <imgui.h>
#include <iostream>
#include <GL/gl3w.h>

#include "graphics/material.h"

#include "default_camera_controller.h"
#include "world/world.h"
#include "engine/asset_manager.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/post_process_pass.h"
#include "world/planet.h"
#include "graphics/primitives.h"
#include "graphics/texture_cube.h"
#include "ui/asset_manager_ui.h"
#include "ui/graphic_debugger.h"
#include "ui/session_frontend.h"
#include "ui/ui.h"
#include "ui/viewport.h"
#include "ui/world_outliner.h"
#include "utils/game_settings.h"
#include "utils/profiler.h"
#include "world/mesh_component.h"

int main()
{
	std::unique_ptr<ActionRecord> main_initialization = std::make_unique<ActionRecord>("main initialization");
	Engine::get().get_renderer().set_icon("resources/textures/icon.png");

	const auto pass_g_buffer_combine = PostProcessPass::create("GBuffer_Combine", Engine::get().get_renderer());
	pass_g_buffer_combine->init("resources/shaders/gbuffer_combine.fs");

	const auto post_process_pass = PostProcessPass::create("PostProcess", Engine::get().get_renderer());
	post_process_pass->init("resources/shaders/post_process.fs");


	ImGuiWindow::create_window<GraphicDebugger>();
	ImGuiWindow::create_window<MaterialManagerUi>();
	ImGuiWindow::create_window<TextureManagerUi>();
	ImGuiWindow::create_window<Viewport>(post_process_pass->result());
	ImGuiWindow::create_window<SessionFrontend>();
	ImGuiWindow::create_window<WorldOutliner>(&Engine::get().get_world());

	// Create planet
	const auto earth = std::make_shared<Planet>("earth");
	earth->set_radius(6000000);
	earth->set_max_lods(19);
	earth->set_cell_count(30);
	Engine::get().get_world().get_scene_root().add_child(earth);

	const auto moon = std::make_shared<Planet>("moon");
	Engine::get().get_world().get_scene_root().add_child(moon);
	moon->set_radius(1700000);
	moon->set_max_lods(18);
	moon->set_cell_count(30);
	moon->set_orbit_distance(30000000.f);
	moon->set_orbit_speed(0.02f);
	moon->set_rotation_speed(0.05f);


	// Create camera controller
	const auto camera_controller = std::make_shared<DefaultCameraController>(Engine::get().get_world().get_camera());
	camera_controller->add_child(Engine::get().get_world().get_camera());
	camera_controller->teleport_to({ 0, 0, earth->get_radius() + 2 });
	earth->add_child(camera_controller);

	const auto default_material = Material::create("standard_material");
	default_material->load_from_source("resources/shaders/standard_material.vs",
	                                   "resources/shaders/standard_material.fs");
	const auto cube = std::make_shared<MeshComponent>("cube");
	cube->set_material(default_material);
	cube->set_mesh(primitives::cube());
	cube->set_local_position({0, 0, earth->get_radius()});
	Engine::get().get_world().get_scene_root().add_child(cube);
	earth->add_child(cube);
	
	const auto cubemap = TextureCube::create("cube map");
	cubemap->from_file("resources/textures/skybox/py.png", "resources/textures/skybox/ny.png",
	                   "resources/textures/skybox/px.png", "resources/textures/skybox/nx.png",
	                   "resources/textures/skybox/pz.png", "resources/textures/skybox/nz.png");

	std::vector<std::shared_ptr<PostProcessPass>> downsample_passes;
	for (int i = 0; i < 9; ++i)
	{
		const auto downsample_pass = PostProcessPass::create("DownSample_" + std::to_string(i),
		                                                     Engine::get().get_renderer());
		downsample_pass->init("resources/shaders/post_process/downsample_pass.fs");
		downsample_pass->on_resolution_changed([i](int& x, int& y)
		{
			x /= static_cast<int>(std::pow(2, i + 1));
			y /= static_cast<int>(std::pow(2, i + 1));
		});
		downsample_passes.emplace_back(downsample_pass);
	}

	std::vector<std::shared_ptr<PostProcessPass>> upsample_passes;
	for (int i = 0; i < 9; ++i)
	{
		const auto upsample_pass = PostProcessPass::create("UpSample_" + std::to_string(i),
		                                                   Engine::get().get_renderer());
		upsample_pass->init("resources/shaders/post_process/upsample_pass.fs");
		upsample_pass->on_resolution_changed([i](int& x, int& y)
		{
			x /= static_cast<int>(std::pow(2, i));
			y /= static_cast<int>(std::pow(2, i));
		});
		upsample_passes.emplace_back(upsample_pass);
	}

	main_initialization = nullptr;
	while (!Engine::get().get_renderer().should_close())
	{
		Engine::get().get_asset_manager().refresh_dirty_assets();
		{
			STAT_FRAME("Game_loop");
			Engine::get().get_renderer().initialize();

			// Gameplay
			Engine::get().get_world().tick_world();

			// Rendering

			// G_buffers
			{
				STAT_FRAME("Deferred GBuffers");
				Engine::get().get_renderer().bind_g_buffers();
				Engine::get().get_world().render_world();
			}

			// Deferred combine
			{
				STAT_FRAME("Deferred combine");
				pass_g_buffer_combine->bind();
				glUniform1f(pass_g_buffer_combine->material()->binding("z_near"),
				            static_cast<float>(Engine::get().get_world().get_camera()->z_near()));
				pass_g_buffer_combine->material()->bind_texture_ex(Engine::get().get_renderer().world_color(),
				                                                   "GBUFFER_color");
				pass_g_buffer_combine->material()->bind_texture_ex(Engine::get().get_renderer().world_normal(),
				                                                   "GBUFFER_normal");
				pass_g_buffer_combine->material()->bind_texture_ex(Engine::get().get_renderer().world_mrao(),
				                                                   "GBUFFER_mrao");
				pass_g_buffer_combine->material()->bind_texture_ex(Engine::get().get_renderer().world_depth(),
				                                                   "GBUFFER_depth");
				pass_g_buffer_combine->material()->bind_texture(cubemap, "WORLD_Cubemap");
				glUniform1f(pass_g_buffer_combine->material()->binding("gamma"), GameSettings::get().gamma);
				glUniform1f(pass_g_buffer_combine->material()->binding("exposure"), GameSettings::get().exposure);
				pass_g_buffer_combine->draw();
			}
			{
				STAT_FRAME("Bloom");
				// Down Samples
				{
					downsample_passes[0]->bind();
					glUniform2f(downsample_passes[0]->material()->binding("input_resolution"),
					            static_cast<float>(pass_g_buffer_combine->width()),
					            static_cast<float>(pass_g_buffer_combine->height()));
					downsample_passes[0]->material()->bind_texture_ex(pass_g_buffer_combine->result(), "Color");
					downsample_passes[0]->draw();
					for (int i = 1; i < downsample_passes.size(); ++i)
					{
						downsample_passes[i]->bind();
						downsample_passes[i]->material()->bind_texture_ex(downsample_passes[i - 1]->result(), "Color");
						glUniform2f(downsample_passes[i]->material()->binding("input_resolution"),
						            static_cast<float>(downsample_passes[i - 1]->width()),
						            static_cast<float>(downsample_passes[i - 1]->height()));
						downsample_passes[i]->draw();
					}
				}
				// Up Samples
				{
					upsample_passes.back()->bind();
					glUniform2f(upsample_passes.back()->material()->binding("input_resolution"),
					            static_cast<float>(downsample_passes.back()->width()),
					            static_cast<float>(downsample_passes.back()->height()));
					glUniform1f(upsample_passes.back()->material()->binding("bloom_strength"),
					            GameSettings::get().bloom_intensity);
					glUniform1f(upsample_passes.back()->material()->binding("step"),
					            1 - (static_cast<float>(upsample_passes.size()) - 1) / static_cast<float>(
						            upsample_passes.size()));
					upsample_passes.back()->material()->bind_texture_ex(downsample_passes.back()->result(),
					                                                    "LastSample");
					upsample_passes.back()->material()->bind_texture_ex(
						downsample_passes[downsample_passes.size() - 2]->result(), "Color");
					upsample_passes.back()->draw();
					for (int i = static_cast<int>(upsample_passes.size()) - 2; i >= 0; --i)
					{
						upsample_passes[i]->bind();
						glUniform2f(upsample_passes[i]->material()->binding("input_resolution"),
						            static_cast<float>(upsample_passes[i + 1]->width()),
						            static_cast<float>(upsample_passes[i + 1]->height()));
						glUniform1f(upsample_passes[i]->material()->binding("bloom_strength"),
						            GameSettings::get().bloom_intensity);
						glUniform1f(upsample_passes[i]->material()->binding("step"),
						            1 - i / static_cast<float>(upsample_passes.size()));
						upsample_passes[i]->material()->bind_texture_ex(upsample_passes[i + 1]->result(), "LastSample");
						upsample_passes[i]->material()->bind_texture_ex(
							i == 0 ? pass_g_buffer_combine->result() : downsample_passes[i - 1]->result(), "Color");
						upsample_passes[i]->draw();
					}
				}
			}
			// Post process
			{
				STAT_FRAME("Post process");
				post_process_pass->bind(Engine::get().get_renderer().is_fullscreen());
				glUniform1f(post_process_pass->material()->binding("gamma"), GameSettings::get().gamma);
				glUniform1f(post_process_pass->material()->binding("exposure"), GameSettings::get().exposure);
				post_process_pass->material()->bind_texture_ex(upsample_passes[0]->result(), "SceneBloom");
				post_process_pass->material()->bind_texture_ex(upsample_passes[0]->result(), "SceneColor");
				post_process_pass->draw();
			}
			// UI
			ui::draw();

			Engine::get().get_renderer().submit();
		}
		Profiler::get().new_frame();
	}
}
