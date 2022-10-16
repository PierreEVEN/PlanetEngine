#include "graphics/camera.h"
#include <imgui.h>
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
#include "utils/profiler.h"
#include "world/mesh_component.h"

int main()
{
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
	earth->radius = 6000000;
	earth->num_lods = 19;
	earth->cell_count = 30;
	earth->regenerate();
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
	                   "resources/textures/skybox/pz.png", "resources/textures/skybox/nz.png");

	std::vector<std::shared_ptr<PostProcessPass>> downsample_passes;
	for (int i = 0; i < 8; ++i)
	{
		const auto downsample_pass = PostProcessPass::create("DownSample_" + std::to_string(i), Engine::get().get_renderer());
		downsample_pass->init("resources/shaders/post_process/downsample_pass.fs");
		downsample_pass->on_resolution_changed([i](int& x, int& y)
			{
				x /= static_cast<int>(std::pow(2, i + 1));
				y /= static_cast<int>(std::pow(2, i + 1));
			});
		downsample_passes.emplace_back(downsample_pass);
	}

	std::vector<std::shared_ptr<PostProcessPass>> upsample_passes;
	for (int i = 0; i < 7; ++i)
	{
		const auto upsample_pass = PostProcessPass::create("UpSample_" + std::to_string(i), Engine::get().get_renderer());
		upsample_pass->init("resources/shaders/post_process/upsample_pass.fs");
		upsample_pass->on_resolution_changed([i](int& x, int& y)
			{
				x /= static_cast<int>(std::pow(2, i + 1));
				y /= static_cast<int>(std::pow(2, i + 1));
			});
		upsample_passes.emplace_back(upsample_pass);
	}

	float bloom_strength = 0.2;
	int bloom_quality = 5;
	float gamma = 2.2;
	float exposure = 1.0;

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
				Eigen::Vector3d(std::cos(moon_orbit), 0, std::sin(moon_orbit)) * 30000000);
			moon->set_local_rotation(
				Eigen::Quaterniond(Eigen::AngleAxisd(moon_rotation, Eigen::Vector3d::UnitY())));
			earth->set_local_rotation(
				Eigen::Quaterniond(Eigen::AngleAxisd(hearth_rotation, Eigen::Vector3d::UnitY())));

			// Rendering

			// G_buffers
			{
				STAT_DURATION("Deferred GBuffers");
				Engine::get().get_renderer().bind_g_buffers();
				Engine::get().get_world().render_world();
			}

			// Deferred combine
			{
				pass_g_buffer_combine->bind();
				glUniform1f(pass_g_buffer_combine->material()->binding("z_near"),
				            static_cast<float>(Engine::get().get_world().get_camera()->z_near()));
				pass_g_buffer_combine->material()->bind_texture_ex(Engine::get().get_renderer().world_color(),
				                                                   "GBUFFER_color");
				pass_g_buffer_combine->material()->bind_texture_ex(Engine::get().get_renderer().world_normal(),
				                                                   "GBUFFER_normal");
				pass_g_buffer_combine->material()->bind_texture_ex(Engine::get().get_renderer().world_depth(),
				                                                   "GBUFFER_depth");
				pass_g_buffer_combine->material()->bind_texture(cubemap,"WORLD_Cubemap");
				pass_g_buffer_combine->draw();
			}
			// Down Samples
			{
				downsample_passes[0]->bind();
				glUniform2f(downsample_passes[0]->material()->binding("input_resolution"), pass_g_buffer_combine->width(), pass_g_buffer_combine->height());
				downsample_passes[0]->material()->bind_texture_ex(pass_g_buffer_combine->result(), "Color");
				downsample_passes[0]->draw();
				for (int i = 1; i < downsample_passes.size(); ++i)
				{
					downsample_passes[i]->bind();
					downsample_passes[i]->material()->bind_texture_ex(downsample_passes[i - 1]->result(), "Color");
					glUniform2f(downsample_passes[i]->material()->binding("input_resolution"), downsample_passes[i - 1]->width(), downsample_passes[i - 1]->height());
					downsample_passes[i]->draw();
				}
			}
			// Up Samples
			{
				ImGui::SliderFloat("bloom strength", &bloom_strength, 0, 1);
				ImGui::SliderInt("bloom quality", &bloom_quality, 1, 20);
				upsample_passes.back()->bind();
				glUniform2f(upsample_passes.back()->material()->binding("input_resolution"), downsample_passes.back()->width(), downsample_passes.back()->height());
				glUniform1f(upsample_passes.back()->material()->binding("bloom_strength"), bloom_strength);
				glUniform1f(upsample_passes.back()->material()->binding("step"), 1 - ((float)upsample_passes.size() - 1) / (float)upsample_passes.size());
				glUniform1i(upsample_passes.back()->material()->binding("bloom_quality"), bloom_quality);
				upsample_passes.back()->material()->bind_texture_ex(downsample_passes.back()->result(), "LastSample");
				upsample_passes.back()->material()->bind_texture_ex(pass_g_buffer_combine->result(), "Color");
				upsample_passes.back()->draw();
				for (int i = upsample_passes.size() - 2; i >= 0; --i)
				{
					upsample_passes[i]->bind();
					glUniform2f(upsample_passes[i]->material()->binding("input_resolution"), upsample_passes[i + 1]->width(), upsample_passes[i + 1]->height());
					glUniform1f(upsample_passes[i]->material()->binding("bloom_strength"), bloom_strength);
					glUniform1f(upsample_passes[i]->material()->binding("step"), 1 - i / (float)upsample_passes.size());
					glUniform1i(upsample_passes[i]->material()->binding("bloom_quality"), bloom_quality);
					upsample_passes[i]->material()->bind_texture_ex(upsample_passes[i + 1]->result(), "LastSample");
					upsample_passes[i]->material()->bind_texture_ex(pass_g_buffer_combine->result(), "Color");
					upsample_passes[i]->draw();
				}
			}

			// Post process
			{
				ImGui::SliderFloat("Exposure", &exposure, 0.1, 4);
				ImGui::SliderFloat("Gamma", &gamma, 0.5, 4);
				post_process_pass->bind(Engine::get().get_renderer().is_fullscreen());
				glUniform1f(post_process_pass->material()->binding("gamma"), gamma);
				glUniform1f(post_process_pass->material()->binding("exposure"), exposure);
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
