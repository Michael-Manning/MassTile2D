#pragma once

#include <glm/glm.hpp>

// helpers to create structures with types that align to std140

union std140_scalerVec {
	uint32_t i;
	float f;
	glm::vec2 v2;
	glm::vec3 v3;
	glm::vec4 v4;
};

struct std140_mat2 {
	glm::vec4 c1;
	glm::vec4 c2;

	std140_mat2() {
		c1 = glm::vec4(0);
		c2 = glm::vec4(0);
	};
	std140_mat2(const glm::mat2 mat) {
		c1 = glm::vec4(mat[0], 0.0f, 0.0f);
		c2 = glm::vec4(mat[1], 0.0f, 0.0f);
	};
};

struct std140_mat3 {
	glm::vec4 c1;
	glm::vec4 c2;
	glm::vec4 c3;

	std140_mat3() {
		c1 = glm::vec4(0);
		c2 = glm::vec4(0);
		c3 = glm::vec4(0);
	};
	std140_mat3(const glm::mat3 mat) {
		c1 = glm::vec4(mat[0], 0.0f);
		c2 = glm::vec4(mat[1], 0.0f);
		c3 = glm::vec4(mat[2], 0.0f);
	};
};

#define std140_mat4 alignas(16) glm::mat4

using std140_TexSmplrID = int32_t;

using std140_int = int;
using std140_float = float;

#define std140_vec2 alignas(8)  glm::vec2
#define std140_vec3 alignas(16) glm::vec3
#define std140_vec4 alignas(16) glm::vec4

using std140_intArray = std140_scalerVec;
using std140_floatArray = std140_scalerVec;
using std140_vec2Array = std140_scalerVec;
using std140_vec3Array = std140_scalerVec;
using std140_vec4Array = glm::vec4;
using std140_textureArray = std140_scalerVec;

struct coordinateTransformUBO_s {
	glm::vec2 position;
	float zoom;
	float aspectRatio = 1.0f;
};