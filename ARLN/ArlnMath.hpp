#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/simd/matrix.h>
#include "ArlnTypes.hpp"

namespace arln {

    template<i32 L, typename T>
    using vec = glm::vec<L, T>;

    using vec2 = vec<2, f32>;
    using vec3 = vec<3, f32>;
    using vec4 = vec<4, f32>;

    using ivec2 = vec<2, i32>;
    using ivec3 = vec<3, i32>;
    using ivec4 = vec<4, i32>;

    using uvec2 = vec<2, u32>;
    using uvec3 = vec<3, u32>;
    using uvec4 = vec<4, u32>;

    template<i32 L, i32 R, typename T>
    using mat = glm::mat<L, R, T>;

    using mat2 = mat<2, 2, f32>;
    using mat3 = mat<3, 3, f32>;
    using mat4 = mat<4, 4, f32>;

    using imat2 = mat<2, 2, i32>;
    using imat3 = mat<3, 3, i32>;
    using imat4 = mat<4, 4, i32>;

    using umat2 = mat<2, 2, u32>;
    using umat3 = mat<3, 3, u32>;
    using umat4 = mat<4, 4, u32>;

    template<typename T>
    constexpr T HalfPi = glm::half_pi<T>();
    template<typename T>
    constexpr T Pi = glm::pi<T>();
    template<typename T>
    constexpr T TwoPi = glm::two_pi<T>();

    template<typename T>
    auto normalize(T const& t_v) -> T
    {
        return glm::normalize(t_v);
    }

    template<typename T>
    auto length(T const& t_v) -> f32
    {
        return glm::length(t_v);
    }

    template<typename T>
    auto dot(T const& t_v1, T const& t_v2) -> f32
    {
        return glm::dot(t_v1, t_v2);
    }

    inline auto toRadians(f32 t_degrees) -> f32
    {
        return glm::radians(t_degrees);
    }

    inline auto toDegrees(f32 t_radians) -> f32
    {
        return glm::degrees(t_radians);
    }

    inline auto rotate(mat4 const& t_matrix, f32 t_angle, vec3 const& t_axis) -> mat4
    {
        return glm::rotate(t_matrix, t_angle, t_axis);
    }

    inline auto rotationMat(vec3 const& t_rotations) -> mat4
    {
        return glm::yawPitchRoll(t_rotations.y, t_rotations.x, t_rotations.z);
    }

    inline auto rotationAngles(mat4 const& t_matrix) -> vec3
    {
        vec3 result{ 0.0f, 0.0f, 0.0f };
        glm::extractEulerAngleXYZ(t_matrix, result.x, result.y, result.z);
        return result;
    }

    inline auto scaleMat(vec3 const& t_scale) -> mat4
    {
        mat4 result{ 1.0f };
        result[0][0] = t_scale.x;
        result[1][1] = t_scale.y;
        result[2][2] = t_scale.z;
        return result;
    }

    inline auto perspective(f32 t_fov, f32 t_aspect, f32 t_znear, f32 t_zfar) -> mat4
    {
        return glm::perspective(t_fov, t_aspect, t_znear, t_zfar);
    }

    inline auto lookAt(vec3 const& t_position, vec3 const& t_center, vec3 const& t_up) -> mat4
    {
        return glm::lookAt(t_position, t_center, t_up);
    }

    inline auto orthographic(f32 t_xLow, f32 t_xHigh, f32 t_yLow, f32 t_yHigh, f32 t_zLow, f32 t_zHigh) -> mat4
    {
        return glm::ortho(t_xLow, t_xHigh, t_yLow, t_yHigh, t_zLow, t_zHigh);
    }
}