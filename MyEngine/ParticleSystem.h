#pragma once

#include <glm/glm.hpp>

struct ParticleSystem {

	bool burstMode;

	float spawnRate; // particles per second
	int particleCount;

	float particleLifeSpan; // seconds
	float gravity = -10.0f;
};