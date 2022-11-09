#pragma once

#include <Eigen/Dense>

namespace Maths {
template <typename Type, int Size>
Eigen::Matrix<Type, Size, 1> lerp(const Eigen::Matrix<Type, Size, 1>& A, const Eigen::Matrix<Type, Size, 1>& B, Type t) {
    return A * (1 - t) + B * t;
}


template<typename Type>
static Eigen::Quaternion<Type> closest_rotation_to(const Eigen::Quaternion<Type>& from, const Eigen::Matrix<Type, 3, 1>& forward, Type min_delta) {
    const Eigen::Matrix<Type, 3, 1>    current_front = from * Eigen::Matrix<Type, 3, 1>(1, 0, 0);
    const Eigen::Quaternion<Type> delta         = Eigen::Quaternion<Type>::FromTwoVectors(current_front, forward);

    const auto forward_step = delta * Eigen::Matrix<Type, 3, 1>(0, 0, 1);
    const auto left_step    = delta * Eigen::Matrix<Type, 3, 1>(0, 1, 0);

    const double forward_angle = acos(forward_step.dot(Eigen::Matrix<Type, 3, 1>(0, 0, 1)));
    const double right_angle   = acos(left_step.dot(Eigen::Matrix<Type, 3, 1>(0, 1, 0)));

    if (forward_angle > min_delta || right_angle > min_delta)
        return delta * from;

    return from;
}
}
