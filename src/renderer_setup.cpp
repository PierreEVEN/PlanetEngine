#include "renderer_setup.h"

#include "engine/engine.h"
#include "graphics/camera.h"
#include "graphics/cubemap_capture_pass.h"
#include "graphics/material.h"
#include "graphics/post_process_pass.h"
#include "graphics/render_pass.h"
#include "graphics/texture_cube.h"
#include "graphics/texture_image.h"
#include "graphics/framegraph.h"
#include "utils/game_settings.h"
#include "world/planet.h"
#include "world/world.h"

std::shared_ptr<FrameGraph> setup_renderer(const std::shared_ptr<Camera>& main_camera) {

    /*
     * DRAW SCENE
     */
    const auto g_buffer_pass = RenderPass::create("G-Buffers", 1, 1);
    g_buffer_pass->add_attachment("color", ImageFormat::RGB_F16, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("normal", ImageFormat::RGB_F16, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("mrao", ImageFormat::RGB_U8, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("debug", ImageFormat::RGB_U8, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("depths", ImageFormat::Depth_F32, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->on_draw.add_lambda([main_camera, g_buffer_pass] {
        main_camera->viewport_res() = {g_buffer_pass->get_width(), g_buffer_pass->get_height()};
        main_camera->use();
        Engine::get().get_world().render_world(DrawGroup::from<DrawGroup_View>(), main_camera);
    });
    
    /*
     * REFLECTIONS
     */
    const auto ssr_pass = PostProcessPass::create("SSR", 1, 1, "resources/shaders/post_process/screen_space_reflections.fs");
    ssr_pass->link_dependency(g_buffer_pass, {"Input_color", "Input_normal", "Input_mrao", "", "Input_Depth"});
    ssr_pass->on_bind_material.add_lambda([](std::shared_ptr<Material> material) {
        material->set_int("enabled", GameSettings::get().screen_space_reflections ? 1 : 0);
        material->set_float("resolution", GameSettings::get().ssr_quality);
    });

    const auto reflection_capture = CubemapCapturePass::create("reflection capture", 256);
    g_buffer_pass->on_draw.add_lambda([] {
        /*
        Create camera
        for i = 0..6 {
            Engine::get().get_world().render_world(DrawGroup::from<DrawGroup_Reflections>(), camera_size);
        }
        */
    });

    /*
     * LIGHTING
     */
    const auto cubemap = TextureCube::create("cube map");
    cubemap->from_file("resources/textures/skybox/py.png", "resources/textures/skybox/ny.png",
                       "resources/textures/skybox/px.png", "resources/textures/skybox/nx.png",
                       "resources/textures/skybox/pz.png", "resources/textures/skybox/nz.png");
    
    const auto env_cubemap = TextureCube::create("environment cube map");
    env_cubemap->from_file("resources/textures/temp_env_map/ny.jpg", "resources/textures/temp_env_map/py.jpg",
                           "resources/textures/temp_env_map/px.jpg", "resources/textures/temp_env_map/nx.jpg",
                           "resources/textures/temp_env_map/nz.jpg", "resources/textures/temp_env_map/pz.jpg");

    const auto lighting = PostProcessPass::create("lighting", 1, 1, "resources/shaders/gbuffer_combine.fs");
    lighting->link_dependency(g_buffer_pass, {"Input_color", "Input_normal", "Input_mrao", "", "Input_Depth"});
    lighting->link_dependency(ssr_pass);
    lighting->on_bind_material.add_lambda([cubemap, env_cubemap, main_camera](std::shared_ptr<Material> material) {
        material->set_float("z_near", static_cast<float>(main_camera->z_near()));
        material->set_int("enable_atmosphere", GameSettings::get().enable_atmosphere ? 1 : 0);
        material->set_int("atmosphere_quality", GameSettings::get().atmosphere_quality);
        material->set_int("shading", static_cast<int>(GameSettings::get().shading));
        material->set_texture("WORLD_Cubemap", cubemap);
        material->set_texture("ENV_cubemap", env_cubemap);
    });

    /*
     * POST PROCESSING
     */
    std::vector<std::shared_ptr<PostProcessPass>> down_sample_passes;
    for (int i = 0; i < 9; ++i) {
        std::shared_ptr<PostProcessPass> pass = PostProcessPass::create("DownSample_" + std::to_string(i), 1, 1, "resources/shaders/post_process/downsample_pass.fs");
        pass->link_dependency(i == 0 ? lighting : down_sample_passes[i - 1]);
        pass->on_compute_resolution([i](uint32_t& x, uint32_t& y) {
            x /= static_cast<int>(std::pow(2, i + 1));
            y /= static_cast<int>(std::pow(2, i + 1));
        });
        down_sample_passes.emplace_back(pass);
    }

    std::vector<std::shared_ptr<PostProcessPass>> up_sample_passes(9);
    for (int i = 8; i >= 0; --i) {
        std::shared_ptr<PostProcessPass> pass = PostProcessPass::create("UpSample_" + std::to_string(i), 1, 1, "resources/shaders/post_process/upsample_pass.fs");
        pass->link_dependency(i == 8 ? down_sample_passes.back() : up_sample_passes[i + 1], {"InputLast"});
        pass->link_dependency(i == 0 ? lighting : down_sample_passes[i - 1], {"InputRaw"});
        pass->on_compute_resolution([i](uint32_t& x, uint32_t& y) {
            x /= static_cast<int>(std::pow(2, i));
            y /= static_cast<int>(std::pow(2, i));
        });
        size_t                                                                      pass_count = up_sample_passes.size();
        pass->on_bind_material.add_lambda([i, pass_count](std::shared_ptr<Material> material) {
            material->set_float("step", 1 - i / static_cast<float>(pass_count));
            material->set_float("bloom_strength", GameSettings::get().bloom_intensity);
        });
        up_sample_passes[i] = pass;
    }

    const auto final_post_process_pass = PostProcessPass::create("PostProcess", 1, 1, "resources/shaders/post_process/post_process.fs");
    final_post_process_pass->link_dependency(up_sample_passes.front(), {"Input_Color"});
    final_post_process_pass->on_bind_material.add_lambda([](std::shared_ptr<Material> material) {
        material->set_float("gamma", GameSettings::get().gamma);
        material->set_float("exposure", GameSettings::get().exposure);
    });

    return FrameGraph::create("main framegraph", final_post_process_pass);
}
