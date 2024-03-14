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


void LightingComputePL::CreateComputePipeline(const std::vector<uint8_t>& computeSrc_firstPass, const std::vector<uint8_t>& computeSrc_secondPass, const std::vector<uint8_t>& computeSrc_thirdPass) {

	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkBaseLightingUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, baseLightUpdateDB);
	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkBaseLightingUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, blurLightUpdateDB);


	PipelineParameters params;
	params.computeSrcStages = { computeSrc_firstPass, computeSrc_secondPass, computeSrc_thirdPass };

	auto  worldMapFGDeviceBuferRef = world->MapFGBuffer.GetDoubleBuffer();
	auto  worldMapBGDeviceBuferRef = world->MapBGBuffer.GetDoubleBuffer();
	auto  worldMapUpscaleBuferRef = world->MapLightUpscaleBuffer.GetDoubleBuffer();
	auto  worldMapBlurBuferRef = world->MapLightBlurBuffer.GetDoubleBuffer();

	PipelineResourceConfig con;
	con.descriptorInfos.reserve(6);
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &baseLightUpdateDB.buffers, baseLightUpdateDB.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &blurLightUpdateDB.buffers, blurLightUpdateDB.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(1, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapFGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo) * (mapCount)));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(1, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapBGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo) * (mapCount)));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(1, 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapUpscaleBuferRef, world->MapLightUpscaleBuffer.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(1, 3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapBlurBuferRef, world->MapLightBlurBuffer.size));

	con.pushInfo = PushConstantInfo{
		.pushConstantSize = sizeof(TileWorld::lightingSettings_pc),
		.pushConstantShaderStages = vk::ShaderStageFlagBits::eCompute
	};

	pipelines.CreateComputePipeline(params, con);

}

void LightingComputePL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int baseUpdates, int blurUpdates) {

	if (baseUpdates == 0 && blurUpdates == 0)
		return;

	ZoneScoped;

	assert(baseUpdates <= maxChunkBaseLightingUpdatesPerFrame && blurUpdates <= maxChunkBaseLightingUpdatesPerFrame);

	pipelines.BindDescriptorSets(commandBuffer);

	pipelines.UpdatePushConstant(commandBuffer, &world->lightingSettings);

	if (baseUpdates != 0)
	{
		TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting compute");
		pipelines.BindPipelineStage(commandBuffer, 0);
		//pipelines.Dispatch(commandBuffer, { baseUpdates * chunkTileCount, 1, 1 }, { 32, 32, 1 });
		pipelines.DispatchGroups(commandBuffer, { baseUpdates, 1, 1 });
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
		if (world->lightingSettings.upscaleEnabled)
		{
			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting up");
			pipelines.BindPipelineStage(commandBuffer, 1);
			//pipelines.Dispatch(commandBuffer, { blurUpdates * chunkTileCount, 2 * chunkTileCount, 2 * chunkTileCount }, { 32, 32, 1 });
			pipelines.DispatchGroups(commandBuffer, { blurUpdates, 2, 2 });
		}


		if (world->lightingSettings.blurEnabled)
		{
			vk::BufferMemoryBarrier barrier;
			barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.buffer = world->MapLightUpscaleBuffer.buffer;
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

			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting blur");
			pipelines.BindPipelineStage(commandBuffer, 2);

			if (world->lightingSettings.upscaleEnabled)
				pipelines.DispatchGroups(commandBuffer, { blurUpdates, 2, 2 });
				//pipelines.Dispatch(commandBuffer, { blurUpdates * chunkTileCount, 2 * chunkTileCount, 2 * chunkTileCount }, { 32, 32, 1 });
			else
				pipelines.DispatchGroups(commandBuffer, { blurUpdates, 1, 1 });
				//pipelines.Dispatch(commandBuffer, { blurUpdates * chunkTileCount, 1, 1}, { 32, 32, 1 });
		}
	}
}