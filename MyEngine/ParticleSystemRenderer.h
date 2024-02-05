#pragma once
#pragma once

#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>
#include <memory>

#include "typedefs.h"
#include "Component.h"
#include "serialization.h"
//#include "ParticleSystem.h"
#include "ParticleSystemPL.h"

#include <assetPack/common_generated.h>

class ParticleSystemRenderer { // : public Component {
public:

	enum class ParticleSystemSize {
		Small,
		Large
	};

	ParticleSystemSize size = ParticleSystemSize::Small;

	// TODO: maybe make try to remove this default argument to prevent unnecessary CPU allocation if immediately converting to compute driven system
	ParticleSystemRenderer(ParticleSystemSize size){// = ParticleSystemSize::Small) {
		SetSystemSize(size);
	};

	void SetSystemSize(ParticleSystemSize size) {
		this->size = size;

		float spawntimer = 0;
		int particlesToSpawn = 0;

		if (size == ParticleSystemSize::Small) {

			// allocate host memory for particle data
			if (hostParticleBuffer == nullptr) {
				hostParticleBuffer = std::make_unique< ParticleSystemPL::ParticleGroup_small>();

				// initialize all paritcles innactive
				for (size_t i = 0; i < MAX_PARTICLES_SMALL; i++)
				{
					hostParticleBuffer->particles[i].position = glm::vec2(0);
					hostParticleBuffer->particles[i].life = 0.0f;
					hostParticleBuffer->particles[i].velocity = glm::vec2(0);
				}
			}
		}
		else if (size == ParticleSystemSize::Large) {
			if (hostParticleBuffer != nullptr) {
				hostParticleBuffer.reset();
			}
		}
		else {
			assert(false);
		}
	}

	// for CPU simulation only
	// change to smart pointer and dynamically allocate only if running simulation on CPU
	std::unique_ptr<ParticleSystemPL::ParticleGroup_small> hostParticleBuffer = nullptr;

	ParticleSystemPL::ParticleSystemConfiguration configuration;

	float spawntimer = 0;
	int particlesToSpawn = 0;
	void runSimulation(float deltaTime, glm::vec2 spawnOrigin);

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

	bool dirty = true;
	ComponentResourceToken* token = nullptr;
};
