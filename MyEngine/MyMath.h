#pragma once

#include <cmath>

constexpr float PI = 3.141592653589793238462643f;
constexpr double PI_D = 3.141592653589793238462643383279502884;

static float exponentialScale(float input, float minv, float maxv) {
    if (input < 0.0f) return minv;
    if (input > 1.0f) return maxv;
    return minv * std::pow(maxv / minv, input);
}