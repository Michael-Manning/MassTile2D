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
#include "ComputeTemplate.h"
#include "ParticleSystemPL.h"


class ParticleComputePL {
public:


	ParticleComputePL(std::shared_ptr<VKEngine>& engine) :
		pipeline(engine), engine(engine) { }

	void CreateComputePipeline(const std::vector<uint8_t>& compSrc, DeviceBuffer* particleDataBuffer);

	void RecordCommandBuffer(vk::CommandBuffer commandBuffer, float deltaTime, std::vector<int>& systemIndexes, std::vector<int>& systemParticleCounts);

	void UploadInstanceConfigurationData(ParticleSystemPL::ParticleSystemConfiguration& psystem, int index) {
		assert(index < MAX_PARTICLE_SYSTEMS_SMALL);
		sysConfigDB.buffersMapped[engine->currentFrame]->systemConfigurations[index] = psystem;
	}


private:

	// mapped configuration buffer
	struct device_particleConfiguration_ssbo {
		ParticleSystemPL::ParticleSystemConfiguration systemConfigurations[MAX_PARTICLE_SYSTEMS_LARGE];
	};

	MappedDoubleBuffer<device_particleConfiguration_ssbo> sysConfigDB;

	ComputeTemplate pipeline;

	std::shared_ptr<VKEngine> engine;

	DeviceBuffer* particleDataBuffer;

	DeviceBuffer atomicCounterBuffer;
};