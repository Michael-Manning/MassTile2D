#pragma once
#pragma once

#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>
#include <memory>

#include "typedefs.h"
#include "Component.h"
#include "serialization.h"
#include "ParticleSystemPL.h"
#include "ParticleStructures.h"

#include <assetPack/common_generated.h>

class ParticleSystemRenderer { // : public Component {
public:

	enum class Size {
		Small = 0,
		Large
	};

	Size size = Size::Small;

	// TODO: maybe make try to remove this default argument to prevent unnecessary CPU allocation if immediately converting to compute driven system
	ParticleSystemRenderer(Size size){
		SetSystemSize(size);
	};

	// used for copying
	ParticleSystemRenderer(Size size, const ParticleSystemConfiguration& config) {
		configuration = config;
		SetSystemSize(size);
	};

	ParticleSystemRenderer(nlohmann::json& j);
	ParticleSystemRenderer(const AssetPack::ParticleSystemRenderer* p);

	// dont' now how to make this work with a map insertion in scene!
	~ParticleSystemRenderer() {
		if (hostParticleBuffer != nullptr) {
			hostParticleBuffer.reset();
		}

		// must indicate to engine that this particle system is no longer using it's associated
		// device local particle buffer
		if (token != nullptr) {
			token->active = false;
		}
	};

	void SetSystemSize(Size size) {
		this->size = size;

		float spawntimer = 0;
		int particlesToSpawn = 0;

		if (size == Size::Small) {

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
			computeContextDirty = false;
		}
		else if (size == Size::Large) {
			if (hostParticleBuffer != nullptr) {
				hostParticleBuffer.reset();
			}
			computeContextDirty = true;
		}
		else {
			assert(false);
		}
	}

	// for CPU simulation only
	// change to smart pointer and dynamically allocate only if running simulation on CPU
	std::unique_ptr<ParticleSystemPL::ParticleGroup_small> hostParticleBuffer = nullptr;

	ParticleSystemConfiguration configuration;

	float spawntimer = 0;
	int particlesToSpawn = 0;
	void runSimulation(float deltaTime, glm::vec2 spawnOrigin);

	bool computeContextDirty = true;
	ComponentResourceToken* token = nullptr;

	nlohmann::json serializeJson(entityID ID) const;
};
