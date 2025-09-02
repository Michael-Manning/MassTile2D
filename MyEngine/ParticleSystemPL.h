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
#include "ParticleStructures.h"
#include "ShaderTypes.h"

constexpr int MAX_PARTICLES_SMALL = 400;
//constexpr int MAX_PARTICLE_SYSTEMS_SMALL = 20;
constexpr int MAX_PARTICLES_LARGE = 400000;
constexpr int MAX_PARTICLE_SYSTEMS_LARGE = 4;

// shader parity
static_assert(sizeof(ShaderTypes::ParticleGroup_small) == sizeof(ShaderTypes::Particle) * MAX_PARTICLES_SMALL);
static_assert(sizeof(ShaderTypes::ParticleGroup_large) == sizeof(ShaderTypes::Particle) * MAX_PARTICLES_LARGE);

class ParticleSystemPL {
public:

	ParticleSystemPL(VKEngine* engine, int maxSystemsSmall) :
		pipeline(engine), engine(engine), maxSystemsSmall(maxSystemsSmall) { }

	void CreateGraphicsPipeline(const PipelineParameters& params, const DeviceBuffer& deviceParticleDataBuffer);

	// indexes should be within particle size group
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes, std::vector<int>& systemSizes, std::vector<int>& systemParticleCounts);

	void UploadInstanceData(ShaderTypes::ParticleGroup_small& psystem, int index) {
		assert(index < maxSystemsSmall);
		particleDB.buffersMapped[engine->currentFrame]->particleGroups_small[index] = psystem;
	}

	const int maxSystemsSmall;

private:

	// cpu driven mapped particle data
	//struct host_particle_ssbo {
		//ParticleGroup_small particleGroups_small[MAX_PARTICLE_SYSTEMS_SMALL];
	//};


	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<ShaderTypes::ParticalSmallGroupInstanceBuffer> particleDB;

	GraphicsTemplate pipeline;

	VKEngine* engine;
};