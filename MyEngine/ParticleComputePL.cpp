#include "stdafx.h"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "globalBufferDefinitions.h"
#include "ShaderTypes.h"
#include "ParticleSystemPL.h"
#include "ParticleStructures.h"
#include "ParticleComputePL.h"

namespace {
		
	inline float randomNormal() {
		return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	}
}

void ParticleComputePL::CreateComputePipeline(const std::vector<uint8_t>& compSrc, DeviceBuffer& particleDataBuffer) {

	// for pipeline barrier
	this->particleDataBuffer = &particleDataBuffer;

	engine->createMappedBuffer(sizeof(ParticleSystemConfiguration) * MAX_PARTICLE_SYSTEMS_LARGE, vk::BufferUsageFlagBits::eStorageBuffer, sysConfigDB);
	//engine->createMappedBuffer(sizeof(device_particleConfiguration_ssbo), vk::BufferUsageFlagBits::eStorageBuffer, sysConfigDB);

	//atomicCounterBuffer.size = sizeof(atomicCounter_ssbo);

	//engine->createBuffer(
	//	atomicCounterBuffer.size,
	//	vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
	//	VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
	//	atomicCounterBuffer.buffer,
	//	atomicCounterBuffer.allocation,
	//	true);

	engine->CreateDeviceOnlyStorageBuffer(sizeof(ShaderTypes::AtomicCounterBuffer), true, atomicCounterBuffer);

	/*auto deviceDB = particleDataBuffer->GetDoubleBuffer();
	auto atomicDB = atomicCounterBuffer.GetDoubleBuffer();*/

	PipelineParameters params;
	params.computeSrcStages = { compSrc };

	PipelineResourceConfig con;

	con.bufferBindings.push_back(BufferBinding(0, 0, sysConfigDB));
	con.bufferBindings.push_back(BufferBinding(0, 1, particleDataBuffer));
	con.bufferBindings.push_back(BufferBinding(0, 2, atomicCounterBuffer));

	pipeline.CreateComputePipeline(params, con);
}

void ParticleComputePL::RecordCommandBuffer(vk::CommandBuffer commandBuffer, float deltaTime, std::vector<DispatchInfo>& dispatchInfo) {
	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "particle system compute");

	assert(dispatchInfo.size() > 0);

	pipeline.BindPipelineStage(commandBuffer, 0);
	pipeline.BindDescriptorSets(commandBuffer);


	for (auto& info : dispatchInfo)
	{
		pipeline.UpdatePushConstant(commandBuffer, ShaderTypes::ParticleDispatchInfo{
			.systemIndex = info.systemIndex,
			.particlesToSpawn = info.particlesToSpawn,
			.deltaTime = deltaTime,
			.seedX = randomNormal(),
			.seedY = randomNormal(),
			.init = info.init ? 1 : 0,
			.spawnPosition = info.spawnPosition
			});

		// reset atomic counter device buffer
		commandBuffer.fillBuffer(atomicCounterBuffer.buffer, 0, VK_WHOLE_SIZE, 0);

		pipeline.DispatchGrid(commandBuffer, { info.particleCount, 1, 1 }, { 32, 1, 1 });
	}

	// should be unnecessary because they are on different queues which are synchronized

	//vk::BufferMemoryBarrier barrier;
	//barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
	//barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	//barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//barrier.buffer = particleDataBuffer->buffer;
	//barrier.offset = 0;
	//barrier.size = VK_WHOLE_SIZE; // Could be changed to a portion, but I don't know if there would be a benefit

	//commandBuffer.pipelineBarrier(
	//	vk::PipelineStageFlagBits::eComputeShader,
	//	vk::PipelineStageFlagBits::eVertexShader,
	//	static_cast<vk::DependencyFlags>(0),
	//	0, nullptr,
	//	1, &barrier,
	//	0, nullptr
	//);

}