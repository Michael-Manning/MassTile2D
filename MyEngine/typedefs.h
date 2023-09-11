#pragma once

#include <stdint.h>

typedef int32_t texID;
typedef uint32_t entityID;
typedef uint32_t spriteID;
typedef uint32_t fontID;
typedef uint32_t atlasID;
typedef uint32_t colliderID;
typedef uint32_t blockID; // type of block
typedef uint32_t tileID; // specific tile in the texture atlas

#include <glm/glm.hpp>

struct Camera {
	glm::vec2 position = glm::vec2(0.0f);
	float zoom = 1.0f;
};

enum class FilterMode {
	Nearest,
	Linear
};