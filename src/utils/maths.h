#pragma once

#include <Eigen/Dense>

namespace Maths
{
	template<typename Type, int Size>
	Eigen::Vector<Type, Size> lerp(const Eigen::Vector<Type, Size>& A, const Eigen::Vector<Type, Size>& B, Type t) {
		return A * (1 - t) + B * t;
	}
}