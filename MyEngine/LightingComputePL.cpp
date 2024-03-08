#include "stdafx.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <fstream>
#include <chrono>
#include <memory>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <tracy/Tracy.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VKengine.h"
#include "typedefs.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"
#include "Vertex.h"
#include "ComputeTemplate.h"

#include "LightingComputePL.h"

using namespace glm;
using namespace std;

void LightingComputePL::CreateComputePipeline(const std::vector<uint8_t>& computeSrc_firstPass, const std::vector<uint8_t>& computeSrc_secondPass) {

	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkBaseLightingUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, baseLightUpdateDB);
	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkBaseLightingUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, blurLightUpdateDB);

	ShaderResourceConfig con;
	con.computeSrcStages = { computeSrc_firstPass, computeSrc_secondPass };

	auto  worldMapFGDeviceBuferRef = world->MapFGBuffer.GetDoubleBuffer();
	auto  worldMapBGDeviceBuferRef = world->MapBGBuffer.GetDoubleBuffer();

	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &baseLightUpdateDB.buffers, baseLightUpdateDB.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &blurLightUpdateDB.buffers, blurLightUpdateDB.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(1, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapFGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo) * (mapCount)));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(1, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapBGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo) * (mapCount)));

	pipelines.CreateComputePipeline(con);

}

void LightingComputePL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int baseUpdates, int blurUpdates) {

	if (baseUpdates == 0 && blurUpdates == 0)
		return;

	ZoneScoped;

	assert(baseUpdates <= maxChunkBaseLightingUpdatesPerFrame && blurUpdates <= maxChunkBaseLightingUpdatesPerFrame);

	pipelines.BindDescriptorSets(commandBuffer);

	if (baseUpdates != 0)
	{
		TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting compute");
		pipelines.BindPipelineStage(commandBuffer, 0);
		pipelines.Dispatch(commandBuffer, { baseUpdates * chunkTileCount, 1, 1 }, { 32, 32, 1 });
	}

	// first stage writes light values to background layer which second stage reads from. Must synchronize accesss to this buffer.
	if (baseUpdates != 0 && blurUpdates != 0)
	{
		vk::BufferMemoryBarrier barrier;
		barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = world->MapFGBuffer.buffer;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE; // Could be changed to a portion, but I don't know if there would be a benefit

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			static_cast<vk::DependencyFlags>(0),
			0, nullptr,
			1, &barrier,
			0, nullptr
		);
	}

	if (blurUpdates != 0)
	{
		TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting blur");
		pipelines.BindPipelineStage(commandBuffer, 1);
		pipelines.Dispatch(commandBuffer, { blurUpdates * chunkTileCount, 1, 1 }, { 32, 32, 1 });
	}
}