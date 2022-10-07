#pragma once

#include <Eigen/Dense>

namespace Maths
{
	template<typename Type, int Size>
	Eigen::Matrix<Type, Size, 1> lerp(const Eigen::Matrix<Type, Size, 1>& A, const Eigen::Matrix<Type, Size, 1>& B, Type t) {
		return A * (1 - t) + B * t;
	}

}