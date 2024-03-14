#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <string>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <nlohmann/json.hpp>

#include <assetPack/common_generated.h>

#include "texture.h"
#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "BindingManager.h"
#include "GlobalImageDescriptor.h"
#include "globalBufferDefinitions.h"
#include "GraphicsTemplate.h"

constexpr int MAX_PARTICLES_SMALL = 400;
constexpr int MAX_PARTICLE_SYSTEMS_SMALL = 20;
constexpr int MAX_PARTICLES_LARGE = 400000;
constexpr int MAX_PARTICLE_SYSTEMS_LARGE = 4;


class ParticleSystemPL {
public:

	struct alignas(16) particle {
		glm::vec2 position;
		glm::vec2 velocity;
		float scale;
		float life;
		alignas(16) glm::vec4 color;
		//int32_t padding[2];
	};

	struct ParticleSystemConfiguration {
		int32_t particleCount = 200;
		bool burstMode = false;
		bool burstRepeat = false;
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

		nlohmann::json serializeJson() const;
		static void deserializeJson(nlohmann::json j, ParticleSystemConfiguration* config);
		static void deserializeFlatbuffers(const AssetPack::ParticleSystemConfiguration* p, ParticleSystemConfiguration* config);
	};

	// for cpu driven particle systems
	struct ParticleGroup_small {
		particle particles[MAX_PARTICLES_SMALL];
	};

	// device/compute driven particle system
	struct ParticleGroup_large{
		particle particles[MAX_PARTICLES_LARGE];
	};


	ParticleSystemPL(VKEngine* engine) :
		pipeline(engine), engine(engine) { }

	void CreateGraphicsPipeline(const PipelineParameters& params, DeviceBuffer* deviceParticleDataBuffer);

	// indexes should be within particle size group
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes, std::vector<int>& systemSizes, std::vector<int>& systemParticleCounts);

	void UploadInstanceData(ParticleGroup_small& psystem, int index) {
		assert(index < MAX_PARTICLE_SYSTEMS_SMALL);
		particleDB.buffersMapped[engine->currentFrame]->particleGroups_small[index] = psystem;
	}

	// device local particle data
	struct device_particle_ssbo{
		ParticleGroup_large particleGroups_large[MAX_PARTICLE_SYSTEMS_LARGE];
	};

private:

	// cpu driven mapped particle data
	struct host_particle_ssbo {
		ParticleGroup_small particleGroups_small[MAX_PARTICLE_SYSTEMS_SMALL];
	};


	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<host_particle_ssbo> particleDB;

	GraphicsTemplate pipeline;

	VKEngine* engine;
};