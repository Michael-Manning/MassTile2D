#pragma once

#include <cmath>
#include <glm/glm.hpp>

constexpr float PI = 3.141592653589793238462643f;
constexpr double PI_D = 3.141592653589793238462643383279502884;

static float exponentialScale(float input, float minv, float maxv) {
    if (input < 0.0f) return minv;
    if (input > 1.0f) return maxv;
    return minv * std::pow(maxv / minv, input);
}

static glm::vec2 smoothstep(const glm::vec2& edge0, const glm::vec2& edge1, float x) {
    return glm::mix(edge0, edge1, glm::smoothstep(0.0f, 1.0f, x));
}

static float magnitude(const glm::vec2& v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

static constexpr glm::vec2 lerp(const glm::vec2 p0, const glm::vec2 p1, const float t) {
    return p0 + t * (p1 - p0);
};

static constexpr float lerp(const float p0, const float p1, const float t) {
    return p0 + t * (p1 - p0);
};