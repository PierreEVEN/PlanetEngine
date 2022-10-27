
#include "graphics/material.h"

#include "default_camera_controller.h"
#include "world/world.h"
#include "engine/asset_manager.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/framegraph.h"
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
#include "graphics/camera.h"

#include <fbo.h>

int main() {

    std::unique_ptr<ActionRecord> main_initialization = std::make_unique<ActionRecord>("main initialization");
    Engine::get().get_renderer().set_icon("resources/textures/icon.png");

    const auto g_buffer_pass = RenderPass::create("G-Buffers", 1, 1);
    g_buffer_pass->add_attachment("color", ImageFormat::RGB_F16, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("normal", ImageFormat::RGB_F16, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("mrao", ImageFormat::RGB_U8, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("depths", ImageFormat::Depth_F32, {.filtering_min = TextureMinFilter::Nearest});

    const auto lighting = PostProcessPassV2::create("lighting", 1, 1, "resources/shaders/gbuffer_combine.fs");
    lighting->link_dependency(g_buffer_pass);
    
    const auto ssr_pass = PostProcessPassV2::create("SSR", 1, 1, "resources/shaders/post_process/screen_space_reflections.fs");
    ssr_pass->link_dependency(g_buffer_pass);

    const auto ssr_combine_pass = PostProcessPassV2::create("SSR_Combine", 1, 1, "resources/shaders/post_process/ssr_combine.fs");
    ssr_combine_pass->link_dependency(lighting);
    ssr_combine_pass->link_dependency(ssr_pass);

    std::vector<std::shared_ptr<PostProcessPassV2>> down_sample_passes;
    for (int i = 0; i < 9; ++i) {
        std::shared_ptr<PostProcessPassV2> pass = PostProcessPassV2::create("DownSample_" + std::to_string(i), 1, 1, "resources/shaders/post_process/downsample_pass.fs");
        pass->link_dependency(i == 0 ? ssr_combine_pass : down_sample_passes[i - 1]);
        pass->on_compute_resolution([i](uint32_t& x, uint32_t& y) {
            x /= static_cast<int>(std::pow(2, i + 1));
            y /= static_cast<int>(std::pow(2, i + 1));
        });
        down_sample_passes.emplace_back(pass);
    }

    std::vector<std::shared_ptr<PostProcessPassV2>> up_sample_passes(9);
    for (int i = 8; i >= 0; --i) {
        std::shared_ptr<PostProcessPassV2> pass = PostProcessPassV2::create("UpSample_", 1, 1, "resources/shaders/post_process/upsample_pass.fs");
        pass->link_dependency(i == 8 ? down_sample_passes.back() : up_sample_passes[i + 1]);
        pass->link_dependency(i == 0 ? ssr_combine_pass : down_sample_passes[i - 1]);
        pass->on_compute_resolution([i](uint32_t& x, uint32_t& y) {
            x /= static_cast<int>(std::pow(2, i));
            y /= static_cast<int>(std::pow(2, i));
        });
        up_sample_passes[i] = pass;
    }
    //@TODO : add named bind points

    const auto final_post_process_pass = PostProcessPassV2::create("PostProcess", 1, 1, "resources/shaders/post_process/post_process.fs");
    final_post_process_pass->link_dependency(ssr_combine_pass);
    final_post_process_pass->link_dependency(up_sample_passes.front());

    const auto framegraph = FrameGraph::create("main framegraph", final_post_process_pass);

    g_buffer_pass->on_draw.add_lambda([] { Engine::get().get_world().render_world(); });

    // Create post process framegraph
    const auto pass_g_buffer_combine = PostProcessPass::create("GBuffer_Combine", Engine::get().get_renderer());
    pass_g_buffer_combine->init("resources/shaders/gbuffer_combine.fs");

    /*
    const auto ssr_pas = PostProcessPass::create("SSR", pass_g_buffer_combine);
    ssr_pas->init("resources/shaders/post_process/screen_space_reflections.fs");

    const auto ssr_combine = PostProcessPass::create("SSR Combine", pass_g_buffer_combine);
    ssr_combine->init("resources/shaders/post_process/ssr_combine.fs");

    std::vector<std::shared_ptr<PostProcessPass>> downsample_passes;
    for (int i = 0; i < 9; ++i) {
        std::shared_ptr<PostProcessPass> pass = nullptr;
        if (i == 0)
            pass = PostProcessPass::create("DownSample_" + std::to_string(i), ssr_combine);
        else
            pass = PostProcessPass::create("DownSample_" + std::to_string(i), downsample_passes[i - 1]);
        pass->init("resources/shaders/post_process/downsample_pass.fs");
        pass->on_resolution_changed([i](int& x, int& y) {
            x /= static_cast<int>(std::pow(2, i + 1));
            y /= static_cast<int>(std::pow(2, i + 1));
        });
        downsample_passes.emplace_back(pass);
    }

    std::vector<std::shared_ptr<PostProcessPass>> upsample_passes(9);
    for (int i = 8; i >= 0; --i) {
        std::shared_ptr<PostProcessPass> pass = nullptr;
        if (i == 8)
            pass = PostProcessPass::create("UpSample_" + std::to_string(i), downsample_passes.back());
        else
            pass = PostProcessPass::create("UpSample_" + std::to_string(i), upsample_passes[i + 1]);
        pass->init("resources/shaders/post_process/upsample_pass.fs");
        pass->on_resolution_changed([i](int& x, int& y) {
            x /= static_cast<int>(std::pow(2, i));
            y /= static_cast<int>(std::pow(2, i));
        });
        upsample_passes[i] = pass;
    }

    const auto post_process_pass = PostProcessPass::create("PostProcess", ssr_combine);
    post_process_pass->init("resources/shaders/post_process/post_process.fs");
    ImGuiWindow::create_window<Viewport>(post_process_pass->result());
    */
    
    ImGuiWindow::create_window<GraphicDebugger>();
    ImGuiWindow::create_window<MaterialManagerUi>();
    ImGuiWindow::create_window<TextureManagerUi>();
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
    camera_controller->teleport_to({0, 0, earth->get_radius() + 2});
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

    main_initialization = nullptr;
    framegraph->resize(Engine::get().get_renderer().framebuffer()->width(), Engine::get().get_renderer().framebuffer()->height());
    while (!Engine::get().get_renderer().should_close()) {
        Engine::get().get_asset_manager().refresh_dirty_assets();
        {
            STAT_FRAME("Game_loop");
            Engine::get().get_renderer().initialize();

            // Gameplay
            Engine::get().get_world().tick_world();

            // Rendering
            framegraph->render();
            
            // Deferred combine
            /*
            {
                STAT_FRAME("Deferred combine");
                pass_g_buffer_combine->bind();
                glUniform1f(pass_g_buffer_combine->material()->binding("z_near"), static_cast<float>(Engine::get().get_world().get_camera()->z_near()));
                glUniform1f(pass_g_buffer_combine->material()->binding("gamma"), GameSettings::get().gamma);
                glUniform1f(pass_g_buffer_combine->material()->binding("exposure"), GameSettings::get().exposure);
                pass_g_buffer_combine->draw();
            }
            {
                STAT_FRAME("Screen space reflections");
                ssr_pas->bind();
                ssr_pas->material()->bind_texture_ex(Engine::get().get_renderer().world_normal(),
                                                     "NormalMap");
                ssr_pas->material()->bind_texture_ex(Engine::get().get_renderer().world_depth(),
                                                     "DepthMap");
                ssr_pas->material()->bind_texture_ex(Engine::get().get_renderer().world_mrao(), "GBUFFER_mrao");
                glUniform1f(ssr_pas->material()->binding("z_near"),
                            static_cast<float>(Engine::get().get_world().get_camera()->z_near()));
                ssr_pas->draw();

                ssr_combine->bind();
                ssr_combine->material()->bind_texture_ex(ssr_pas->result(), "SSR_Coordinates");
                ssr_combine->material()->bind_texture_ex(Engine::get().get_renderer().world_mrao(), "GBUFFER_mrao");
                ssr_combine->draw();
            }
            {
                STAT_FRAME("Bloom");
                // Down Samples
                {
                    for (int i = 0; i < downsample_passes.size(); ++i) {
                        downsample_passes[i]->bind();
                        downsample_passes[i]->draw();
                    }
                }
                // Up Samples
                {
                    for (int i = static_cast<int>(upsample_passes.size()) - 1; i >= 0; --i) {
                        upsample_passes[i]->bind();
                        glUniform1f(upsample_passes[i]->material()->binding("bloom_strength"),
                                    GameSettings::get().bloom_intensity);
                        glUniform1f(upsample_passes[i]->material()->binding("step"),
                                    1 - i / static_cast<float>(upsample_passes.size()));
                        upsample_passes[i]->material()->bind_texture_ex(
                            i == 0 ? ssr_combine->result() : downsample_passes[i - 1]->result(), "Color");
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
                post_process_pass->material()->bind_texture_ex(upsample_passes.front()->result(), "SceneBloom");
                post_process_pass->draw();
            }
            */
            // UI
            ui::draw();

            Engine::get().get_renderer().submit();
        }
        Profiler::get().new_frame();
    }
}
