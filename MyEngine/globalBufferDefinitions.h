#pragma once

#include <glm/glm.hpp>

struct cameraUBO_s {
	glm::vec2 position;
	float zoom;
	float aspectRatio = 1.0f;
};