#pragma once

#include <vector>
#include <stdint.h>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "ComputeTemplate.h"
#include "ParticleSystemPL.h"
#include "ParticleStructures.h"


class ParticleComputePL {
public:



	struct DispatchInfo{
		int systemIndex;
		int particleCount;
		int particlesToSpawn;
		bool init;
		glm::vec2 spawnPosition;
	};

	ParticleComputePL(VKEngine* engine) :
		pipeline(engine), engine(engine) { }

	void CreateComputePipeline(const std::vector<uint8_t>& compSrc, DeviceBuffer& particleDataBuffer);

	void RecordCommandBuffer(vk::CommandBuffer commandBuffer, float deltaTime, std::vector<DispatchInfo>& dispatchInfo);

	void UploadInstanceConfigurationData(ParticleSystemConfiguration& psystem, int index) {
		assert(index < MAX_PARTICLE_SYSTEMS_LARGE);

		sysConfigDB.buffersMapped[engine->currentFrame][index] = psystem;
	}


private:

	
	MappedDoubleBuffer<ParticleSystemConfiguration> sysConfigDB;

	ComputeTemplate pipeline;

	VKEngine* engine;

	DeviceBuffer* particleDataBuffer;

	DeviceBuffer atomicCounterBuffer;
};