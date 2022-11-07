#pragma once
#include <memory>

class Camera;
class FrameGraph;
std::shared_ptr<FrameGraph> setup_renderer(const std::shared_ptr<Camera>& main_camera);
