#include "world.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <thread>
#include "graphics/camera.h"
#include "graphics/draw_group.h"
#include "utils/game_settings.h"
#include "utils/profiler.h"

World::World()
    : root_component(std::make_unique<SceneComponent>("root")) {
}

void World::tick_world() {
    STAT_FRAME("World tick");

    {
        const double required_delta_s = 1.0 / GameSettings::get().max_fps;
        STAT_FRAME("Framerate limiter");
        do {
            delta_seconds = std::min(glfwGetTime() - last_time, 1.0);
            if (GameSettings::get().max_fps > 1)
                std::this_thread::sleep_for(
                    std::chrono::microseconds(
                        static_cast<size_t>(std::max(0.0, required_delta_s - delta_seconds) * 1000000)));
        } while (GameSettings::get().max_fps > 1 && delta_seconds < required_delta_s);
        last_time = glfwGetTime();
    }

    {
        STAT_FRAME("Pre-Physic");
        root_component->tick_internal(delta_seconds, TickGroup::PrePhysic);
    }
    {
        STAT_FRAME("Physic");
        root_component->tick_internal(delta_seconds, TickGroup::Physic);
    }
    {
        STAT_FRAME("Post-Physic");
        root_component->tick_internal(delta_seconds, TickGroup::PostPhysic);
    }

}

void World::render_world(const DrawGroup& draw_group, const std::shared_ptr<Camera>& render_camera) const {
    STAT_FRAME("World render");
    root_component->render_internal(*render_camera, draw_group);
}
