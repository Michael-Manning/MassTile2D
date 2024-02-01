#pragma once
#pragma once

#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>

#include "typedefs.h"
#include "Component.h"
#include "serialization.h"
#include "ParticleSystem.h"
#include "ParticleSystemPL.h"

#include <assetPack/common_generated.h>

class ParticleSystemRenderer { // : public Component {
public:

	ParticleSystemRenderer() {
		for (size_t i = 0; i < MAX_PARTICLES_MEDIUM; i++)
		{
			particleData.particles->position = glm::vec2(0);
			particleData.particles->life = 0.0f;
			particleData.particles->velocity = glm::vec2(0);
		}
	};
	/*ParticleSystemRenderer() {
	}*/

	particleSystemID particleSystemID;

	// for CPU simulation only
	// change to smart pointer and dynamically allocate only if running simulation on CPU
	ParticleSystemPL::particleSystem particleData;

	float spawntimer = 0;
	int particlesToSpawn = 0;
	void runSimulation(float deltaTime, ParticleSystem& ps, glm::vec2 spawnOrigin);

	//nlohmann::json serializeJson(entityID entId) override;
	//static ColorRenderer deserializeJson(nlohmann::json);

	/*ColorRenderer duplicate() const {
		ColorRenderer r(color, shape);
		return r;
	};

	static ColorRenderer deserializeFlatbuffers(const AssetPack::ColorRenderer* c) {
		ColorRenderer r;
		r.color = fromAP(c->color());
		r.shape = static_cast<Shape>(c->shape());
		return r;
	};*/
};
