#include "default_camera_controller.h"
#include "renderer_setup.h"
#include "engine/asset_manager.h"
#include "engine/engine.h"
#include "engine/renderer.h"
#include "graphics/camera.h"
#include "graphics/framegraph.h"
#include "graphics/material.h"
#include "graphics/render_pass.h"
#include "ui/asset_manager_ui.h"
#include "ui/graphic_debugger.h"
#include "ui/session_frontend.h"
#include "ui/viewport.h"
#include "ui/world_outliner.h"
#include "utils/game_settings.h"
#include "utils/profiler.h"
#include "world/planet.h"
#include "world/planet_ocean.h"
#include "world/world.h"

int main() {
    std::unique_ptr<ActionRecord> main_initialization = std::make_unique<ActionRecord>("main initialization");
    Engine::get().get_renderer().set_icon("resources/textures/icon.png");

    const auto main_camera = std::make_shared<Camera>();

    const auto framegraph = setup_renderer(main_camera);

    const auto viewport = ImGuiWindow::create_window<Viewport>(framegraph->get_root()->get_color_attachments()[0]->get_render_target());
    ImGuiWindow::create_window<GraphicDebugger>(framegraph);
    ImGuiWindow::create_window<MaterialManagerUi>();
    ImGuiWindow::create_window<TextureManagerUi>();
    ImGuiWindow::create_window<SessionFrontend>();
    ImGuiWindow::create_window<WorldOutliner>(&Engine::get().get_world());

    // Create planet
    const auto earth = Planet::create("earth", main_camera);
    Engine::get().get_world().get_scene_root().add_child(earth);
    earth->set_radius(600000);
    earth->set_max_lods(15);
    earth->set_cell_count(30);
    earth->set_rotation_speed(0);

    const auto earth_ocean = std::make_shared<PlanetOcean>(earth);
    earth->add_child(earth_ocean);

    const auto moon = Planet::create("moon", main_camera);
    Engine::get().get_world().get_scene_root().add_child(moon);
    moon->set_radius(170000);
    moon->set_max_lods(14);
    moon->set_cell_count(30);
    moon->set_orbit_distance(3000000.f);
    moon->set_orbit_speed(0.02f);
    moon->set_rotation_speed(0.05f);

    // Create camera controller
    const auto camera_controller = std::make_shared<DefaultCameraController>();
    camera_controller->add_child(main_camera);
    camera_controller->teleport_to({0, 0, earth->get_radius() + 2});
    earth->add_child(camera_controller);

    main_initialization = nullptr;
    while (!Engine::get().get_renderer().should_close()) {
        Engine::get().get_asset_manager().refresh_dirty_assets();
        {
            STAT_FRAME("Game_loop");
            Engine::get().get_renderer().initialize();

            // Gameplay
            Engine::get().get_world().tick_world();

            // Rendering
            if (GameSettings::get().fullscreen)
                framegraph->render(true, Engine::get().get_renderer().window_width(), Engine::get().get_renderer().window_height());
            else
                framegraph->render(false, viewport->width(), viewport->height());

            // UI
            ui::draw();

            Engine::get().get_renderer().submit();
        }
        Profiler::get().new_frame();
    }
}
