#pragma once

#include <stdint.h>
#include <glm/glm.hpp>

#include "nlohmann/json.hpp"
#include "serialization.h"
//
//struct alignas(16) Particle {
//	glm::vec2 position;
//	glm::vec2 velocity;
//	float scale;
//	float life;
//	alignas(16) glm::vec4 color;
//	//int32_t padding[2];
//};

struct ParticleSystemConfiguration {
	int32_t particleCount = 200;
	uint32_t burstMode = false;
	uint32_t burstRepeat = false;
	float spawnRate = 100.0f; // particles per second
	float particleLifeSpan = 1.0f; // seconds
	float gravity = -10.0f;
	float startSize = 0.3;
	//float spawnMinStartSize;
	//float spawnMaxStartSize;
	float endSize = 0.0;
	//float spawnAngle; // When adding this, make sure to incorporate parent transformations/rotation angle
	//float spawnAngleRange;
	//float spawnMinVelocity;
	//float spawnMaxVelocity;
	alignas(16) glm::vec4 startColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	alignas(16) glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);

	ParticleSystemConfiguration() {};

	ParticleSystemConfiguration(const AssetPack::ParticleSystemConfiguration* p) {
		particleCount = p->particleCount();
		burstMode = p->burstMode();
		spawnRate = p->spawnRate();
		particleLifeSpan = p->particleLifeSpan();
		gravity = p->gravity();
		startSize = p->startSize();
		endSize = p->endSize();
		startColor = fromAP(p->startColor());
		endColor = fromAP(p->endColor());
	}

	ParticleSystemConfiguration(nlohmann::json j) {
		particleCount = j["particleCount"];
		burstMode = j["burstMode"];
		spawnRate = j["spawnRate"];
		particleLifeSpan = j["particleLifeSpan"];
		gravity = j["gravity"];
		startSize = j["startSize"];
		endSize = j["endSize"];
		startColor = fromJson<glm::vec4>(j["startColor"]);
		endColor = fromJson<glm::vec4>(j["endColor"]);
	}

	nlohmann::json serializeJson() const {
		nlohmann::json j;

		j["particleCount"] = particleCount;
		j["burstMode"] = burstMode;
		j["spawnRate"] = spawnRate;
		j["particleLifeSpan"] = particleLifeSpan;
		j["gravity"] = gravity;
		j["startSize"] = startSize;
		j["endSize"] = endSize;
		j["startColor"] = toJson(startColor);
		j["endColor"] = toJson(endColor);

		return j;
	}
};