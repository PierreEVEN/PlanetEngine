#pragma once

class World;
class Renderer;

namespace ui
{
	void draw(const Renderer& renderer, const World& world, const std::shared_ptr<EZCOGL::FBO_DepthTexture>& g_buffer);
}
