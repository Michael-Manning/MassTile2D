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

struct pushConstant_s {
	int systemIndex;
};

void ParticleComputePL::CreateComputePipeline(const std::vector<uint8_t>& compSrc, DeviceBuffer* particleDataBuffer) {

	this->particleDataBuffer = particleDataBuffer;

	engine->createMappedBuffer(sizeof(device_particleConfiguration_ssbo), vk::BufferUsageFlagBits::eStorageBuffer, sysConfigDB);

	auto deviceDB = particleDataBuffer->GetDoubleBuffer();

	ShaderResourceConfig con;
	con.computeSrc = compSrc;
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &sysConfigDB.buffers, sysConfigDB.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &deviceDB, particleDataBuffer->size));

	con.pushInfo = PushConstantInfo{
		.pushConstantSize = sizeof(pushConstant_s),
		.pushConstantShaderStages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
	};

	pipeline.CreateComputePipeline(con);
}

void ParticleComputePL::RecordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes, std::vector<int>& systemParticleCounts) {
	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "particle system compute");

	assert(systemIndexes.size() == systemParticleCounts.size());


	pipeline.bindPipelineResources(commandBuffer);

	for (size_t i = 0; i < systemIndexes.size(); i++)
	{
		pushConstant_s pc{ .systemIndex = systemIndexes[i] };
		pipeline.updatePushConstant(commandBuffer, &pc);

		pipeline.Dispatch(commandBuffer, { systemParticleCounts[i], 1, 1 }, { 32, 1, 1 });
	}
}