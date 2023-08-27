#pragma once

#include <stdint.h>

typedef int32_t texID;
typedef uint32_t entityID;
typedef uint32_t spriteID;
typedef uint32_t atlasID;
typedef uint32_t colliderID;
typedef uint32_t blockID;

#include <glm/glm.hpp>

struct Camera {
	glm::vec2 position = glm::vec2(0.0f);
	float zoom = 1.0f;
};

enum class FilterMode {
	Nearest,
	Linear
};