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
constexpr int MAX_PARTICLE_SYSTEMS_SMALL = 10;
constexpr int MAX_PARTICLES_LARGE = 4000;
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
		bool burstMode;
		float spawnRate = 100.0f; // particles per second
		float particleLifeSpan = 1.0f; // seconds
		float gravity = -10.0f;
		float startSize = 0.3; 
		float endSize = 0.0;
		alignas(16) glm::vec4 startColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		alignas(16) glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	};

	// for cpu driven particle systems
	struct ParticleGroup_small {
		particle particles[MAX_PARTICLES_SMALL];
	};

	// device/compute driven particle system
	struct ParticleGroup_large{
		particle particles[MAX_PARTICLES_LARGE];
	};


	ParticleSystemPL(std::shared_ptr<VKEngine>& engine) :
		pipeline(engine), engine(engine) { }

	void CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, MappedDoubleBuffer<cameraUBO_s>& cameradb, bool flipFaces = false, bool transparentFramebuffer = false);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes, std::vector<int>& systemParticleCounts);

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

	std::shared_ptr<VKEngine> engine;
};