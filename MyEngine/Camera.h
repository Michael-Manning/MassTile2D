#pragma once

#include <glm/glm.hpp>

struct Camera {
	glm::vec2 position = glm::vec2(0.0f);
	float zoom = 1.0f;
};