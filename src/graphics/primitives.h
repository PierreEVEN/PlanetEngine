#pragma once
#include <memory>


class Mesh;

namespace primitives
{
	std::shared_ptr<Mesh> cube(float size = 1.0);
}
