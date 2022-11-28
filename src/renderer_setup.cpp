#include "renderer_setup.h"

#include "engine/engine.h"
#include "graphics/camera.h"
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
    const auto g_buffer_pass = RenderPass::create("DeferredScene", 1, 1);
    g_buffer_pass->add_attachment("Scene_color", ImageFormat::RGB_F16, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("Scene_normal", ImageFormat::RGB_F16, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("Scene_mrao", ImageFormat::RGB_U8, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->add_attachment("Scene_depths", ImageFormat::Depth_F32, {.filtering_min = TextureMinFilter::Nearest});
    g_buffer_pass->on_draw.add_lambda([main_camera, g_buffer_pass] {
        main_camera->viewport_res() = {g_buffer_pass->get_width(), g_buffer_pass->get_height()};
        main_camera->use();
        Engine::get().get_world().render_world(DrawGroup::from<DrawGroup_View>(), main_camera, g_buffer_pass);
    });

    /*
     * DRAW TRANSLUCENCY
     */
    // Translucency pass (//@TODO : reuse framebuffer to make multiple translucency pass possible)
    const auto translucency = RenderPass::create("DeferredTranslucency", 1, 1);
    translucency->add_attachment("Translucency_color", ImageFormat::RGBA_F16, {.filtering_min = TextureMinFilter::Nearest});
    translucency->add_attachment("Translucency_normal", ImageFormat::RGB_F16, {.filtering_min = TextureMinFilter::Nearest});
    translucency->add_attachment("Translucency_mrao", ImageFormat::RGB_U8, {.filtering_min = TextureMinFilter::Nearest});
    translucency->add_attachment("Translucency_depth", ImageFormat::Depth_F32, {.filtering_min = TextureMinFilter::Nearest});
    translucency->link_dependency(g_buffer_pass, {"Scene_color", "Scene_normal", "Scene_mrao", "Scene_depth"});
    translucency->on_draw.add_lambda([main_camera, translucency] {
        Engine::get().get_world().render_world(DrawGroup::from<DrawGroup_Translucency>(), main_camera, translucency);
    });

    // Combine translucency with scene
    const auto translucency_combine = PostProcessPass::create("Translucency_combine", 1, 1, "resources/shaders/post_process/translucency_combine.fs");
    translucency_combine->add_attachment("Translucency_normal", ImageFormat::RGB_F16, {.filtering_min = TextureMinFilter::Nearest});
    translucency_combine->add_attachment("Translucency_mrao", ImageFormat::RGB_U8, {.filtering_min = TextureMinFilter::Nearest});
    translucency_combine->add_attachment("Translucency_depth", ImageFormat::R_F32, {.filtering_min = TextureMinFilter::Nearest});
    translucency_combine->link_dependency(g_buffer_pass, {"Scene_color", "Scene_normal", "Scene_mrao", "Scene_depth"});
    translucency_combine->link_dependency(translucency, {"Translucency_color", "Translucency_normal", "Translucency_mrao", "Translucency_depth"});
    translucency_combine->on_bind_material.add_lambda([main_camera](std::shared_ptr<Material> material) {
        material->set_float("z_near", static_cast<float>(main_camera->z_near()));
        material->set_int("shading", static_cast<int>(GameSettings::get().shading));
        material->set_vec3("sun_direction", Eigen::Vector3f((GameSettings::get().sun_direction * Eigen::Vector3d(1, 0, 0)).cast<float>()));
    });

    /*
     * LIGHTING
     */
    const auto cubemap = TextureCube::create("cube map");
    cubemap->from_file("resources/textures/skybox/py.png", "resources/textures/skybox/ny.png",
                       "resources/textures/skybox/px.png", "resources/textures/skybox/nx.png",
                       "resources/textures/skybox/pz.png", "resources/textures/skybox/nz.png");

    const auto lighting = PostProcessPass::create("lighting", 1, 1, "resources/shaders/post_process/lighting.fs");
    lighting->link_dependency(g_buffer_pass, {"Scene_color", "Scene_normal", "Scene_mrao", "Scene_depth"});
    lighting->link_dependency(translucency_combine, {"Translucency_color", "Translucency_normal","Translucency_mrao", "Translucency_depth"});
    lighting->on_bind_material.add_lambda([cubemap, main_camera](std::shared_ptr<Material> material) {
        material->set_float("z_near", static_cast<float>(main_camera->z_near()));
        material->set_int("enable_atmosphere", GameSettings::get().enable_atmosphere ? 1 : 0);
        material->set_int("atmosphere_quality", GameSettings::get().atmosphere_quality);
        material->set_int("shading", static_cast<int>(GameSettings::get().shading));
        material->set_texture("WORLD_Cubemap", cubemap);
        material->set_vec3("sun_direction", Eigen::Vector3f((GameSettings::get().sun_direction * Eigen::Vector3d(1, 0, 0)).cast<float>()));
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
