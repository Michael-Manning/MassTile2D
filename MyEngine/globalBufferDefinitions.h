#pragma once

#include <glm/glm.hpp>


struct std140Mat3 {
	glm::vec4 c1;
	glm::vec4 c2;
	glm::vec4 c3;

	std140Mat3() {
		c1 = glm::vec4(0);
		c2 = glm::vec4(0);

		c3 = glm::vec4(0);
	}
	std140Mat3(const glm::mat3 mat) {
		c1 = glm::vec4(mat[0], 0.0f);
		c2 = glm::vec4(mat[1], 0.0f);
		c3 = glm::vec4(mat[2], 0.0f);
	}
};

struct cameraUBO_s {
	glm::vec2 position;
	float zoom;
	float aspectRatio = 1.0f;
};

struct transformSSBO_430 {
	glm::mat3 transform;
};