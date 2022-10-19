#pragma once

#include <Eigen/Dense>

namespace ui
{
	bool position_edit(Eigen::Vector3d& position, const std::string& title);
	bool rotation_edit(Eigen::Quaterniond& position, const std::string& title);
}