#pragma once
#include "ui.h"

class SceneComponent;

class WorldOutliner : public ImGuiWindow {
public:
    WorldOutliner(const World* in_world)
        : world(in_world) {
        window_name = "World outliner";
    }

    void draw() override;

private:
    bool                            draw_node(const std::shared_ptr<SceneComponent>& node);
    std::shared_ptr<SceneComponent> selected_node = nullptr;
    const World*                    world;
};
