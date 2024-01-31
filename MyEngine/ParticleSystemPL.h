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

constexpr int MAX_PARTICLES_MEDIUM = 1000;
constexpr int MAX_PARTICLE_SYSTEMS_MEDIUM = 10;


class ParticleSystemPL {
public:

	struct alignas(8) particle {
		glm::vec2 position;
		glm::vec2 velocity;
		float scale;
		float life;
		//int32_t padding[2];
	};
	//static_assert(sizeof(particle) % 16 == 0);

	struct particleSystem {

		int32_t particleCount;
		int32_t padding[1];
		particle particles[MAX_PARTICLES_MEDIUM];
	};
	//static_assert(sizeof(particleSystem) % 32 == 0);

	ParticleSystemPL(std::shared_ptr<VKEngine>& engine) :
		pipeline(engine), engine(engine) { }

	void CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, MappedDoubleBuffer<cameraUBO_s>& cameradb, bool flipFaces = false);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes);

	void UploadInstanceData(particleSystem& psystem, int index) {
		assert(index < MAX_PARTICLE_SYSTEMS_MEDIUM);
		particleDB.buffersMapped[engine->currentFrame]->systems[index] = psystem;
	}

private:

	struct particle_ssbo {
		particleSystem systems[MAX_PARTICLE_SYSTEMS_MEDIUM];
	};
	static_assert(sizeof(particle_ssbo) % 16 == 00);

	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<particle_ssbo> particleDB;

	GraphicsTemplate pipeline;

	std::shared_ptr<VKEngine> engine;
};


struct particle {
	glm::vec2 position;
	glm::vec2 velocity;
	float scale;
	float life;
};

struct particleSystem {
	int32_t particleCount;
	particle particles[MAX_PARTICLES_MEDIUM];
};

struct particle_ssbo {
	particleSystem systems[MAX_PARTICLE_SYSTEMS_MEDIUM];
};