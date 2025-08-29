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


class ParticleSystemPL {
public:
	// for cpu driven particle systems
	struct ParticleGroup_small {
		ShaderTypes::Particle particles[MAX_PARTICLES_SMALL];
	};

	ParticleSystemPL(VKEngine* engine, int maxSystemsSmall) :
		pipeline(engine), engine(engine), maxSystemsSmall(maxSystemsSmall) { }

	void CreateGraphicsPipeline(const PipelineParameters& params, const DeviceBuffer& deviceParticleDataBuffer);

	// indexes should be within particle size group
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes, std::vector<int>& systemSizes, std::vector<int>& systemParticleCounts);

	void UploadInstanceData(ParticleGroup_small& psystem, int index) {
		assert(index < maxSystemsSmall);
		particleDB.buffersMapped[engine->currentFrame][index] = psystem;
	}

	const int maxSystemsSmall;

private:

	// cpu driven mapped particle data
	//struct host_particle_ssbo {
		//ParticleGroup_small particleGroups_small[MAX_PARTICLE_SYSTEMS_SMALL];
	//};


	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<ParticleGroup_small> particleDB;

	GraphicsTemplate pipeline;

	VKEngine* engine;
};