#pragma once
#include <memory>


class Mesh;

namespace primitives
{
std::shared_ptr<Mesh> cube(float size = 1.0);
std::shared_ptr<Mesh> grid_plane(int res_x, int res_y, float cell_size = 1);
}
