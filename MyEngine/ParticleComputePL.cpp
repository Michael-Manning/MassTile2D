#include "stdafx.h"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "globalBufferDefinitions.h"
#include "ParticleSystemPL.h"
#include "ParticleComputePL.h"

namespace {

	inline float randomNormal() {
		return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	}

	struct pushConstant_s {
		int32_t systemIndex;
		int32_t particlesToSpawn;
		float deltaTime;
		float seedX;
		float seedY;
		int32_t init;
		alignas(8)glm::vec2 spawnPosition;
	};

	struct atomicCounter_ssbo {
		uint32_t activeCount;
	};
}

void ParticleComputePL::CreateComputePipeline(const std::vector<uint8_t>& compSrc, DeviceBuffer* particleDataBuffer) {

	this->particleDataBuffer = particleDataBuffer;

	engine->createMappedBuffer(sizeof(device_particleConfiguration_ssbo), vk::BufferUsageFlagBits::eStorageBuffer, sysConfigDB);

	atomicCounterBuffer.size = sizeof(atomicCounter_ssbo);

	engine->createBuffer(
		atomicCounterBuffer.size,
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		atomicCounterBuffer.buffer,
		atomicCounterBuffer.allocation,
		true);


	auto deviceDB = particleDataBuffer->GetDoubleBuffer();
	auto atomicDB = atomicCounterBuffer.GetDoubleBuffer();

	PipelineParameters params;
	params.computeSrcStages = { compSrc };

	PipelineResourceConfig con;
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &sysConfigDB.buffers, sysConfigDB.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &deviceDB, particleDataBuffer->size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &atomicDB, atomicCounterBuffer.size));

	con.pushInfo = PushConstantInfo{
		.pushConstantSize = sizeof(pushConstant_s),
		.pushConstantShaderStages = vk::ShaderStageFlagBits::eCompute
	};

	pipeline.CreateComputePipeline(params, con);
}

void ParticleComputePL::RecordCommandBuffer(vk::CommandBuffer commandBuffer, float deltaTime, std::vector<DispatchInfo>& dispatchInfo) {
	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "particle system compute");

	pipeline.BindPipelineStage(commandBuffer, 0);
	pipeline.BindDescriptorSets(commandBuffer);

	// reset atomic counter device buffer
	commandBuffer.fillBuffer(atomicCounterBuffer.buffer, 0, VK_WHOLE_SIZE, 0);

	for (auto& info : dispatchInfo)
	{
		pushConstant_s pc{
			.systemIndex = info.systemIndex,
			.particlesToSpawn = info.particlesToSpawn,
			.deltaTime = deltaTime,
			.seedX = randomNormal(),
			.seedY = randomNormal(),
			.init = info.init ? 1 : 0,
			.spawnPosition = info.spawnPosition
		};
		pipeline.UpdatePushConstant(commandBuffer, &pc);

		//pipeline.Dispatch(commandBuffer, { info.particleCount, 1, 1 }, { 32, 1, 1 });
		pipeline.DispatchGrid(commandBuffer, { info.particleCount, 1, 1 }, { 32, 1, 1 });
	}
}