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

#include "LightingComputePL.h"

using namespace glm;
using namespace std;

void LightingComputePL::CreateComputePipeline(const std::vector<uint8_t>& computeSrc_firstPass, const std::vector<uint8_t>& computeSrc_secondPass) {

	auto firstComputeStage = createComputeShaderStage(computeSrc_firstPass);
	auto secondComputeStage = createComputeShaderStage(computeSrc_secondPass);

	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkBaseLightingUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, baseLightUpdateDB);
	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkBaseLightingUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, blurLightUpdateDB);

	std::array<vk::Buffer, FRAMES_IN_FLIGHT> worldMapFGDeviceBuferRef = { world->_worldMapFGDeviceBuffer, world->_worldMapFGDeviceBuffer };
	std::array<vk::Buffer, FRAMES_IN_FLIGHT> worldMapBGDeviceBuferRef = { world->_worldMapBGDeviceBuffer, world->_worldMapBGDeviceBuffer };
	descriptorManager.configureDescriptorSets(vector<DescriptorManager::descriptorSetInfo> {
		DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &baseLightUpdateDB.buffers, baseLightUpdateDB.size),
		DescriptorManager::descriptorSetInfo(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &blurLightUpdateDB.buffers, blurLightUpdateDB.size),
		DescriptorManager::descriptorSetInfo(1, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapFGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo)* (mapCount)),
		DescriptorManager::descriptorSetInfo(1, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapBGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo)* (mapCount))
	});
	descriptorManager.buildDescriptorLayouts();

	descriptorLayoutMap setLayouts;
	for (auto& [set, layout] : descriptorManager.builderLayouts)
		setLayouts[set] = layout;

	//// create vector containing the builder descriptor set layouts
	//vector< vk::DescriptorSetLayout> setLayouts;
	//setLayouts.reserve(builderLayouts.size());
	//for (auto& [set, layout] : builderLayouts) {
	//	setLayouts.push_back(layout);
	//}
	buildPipelineLayout(setLayouts);

	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.layout = pipelineLayout;

	{
		pipelineInfo.stage = firstComputeStage;
		engine->devContext.device.createComputePipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &firstStagePipeline);
	}
	{
		pipelineInfo.stage = secondComputeStage;
		engine->devContext.device.createComputePipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &secondStagePipeline);
	}

	descriptorManager.buildDescriptorSets();
}

void LightingComputePL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int chunkUpdateCount) {




	// Should store two compute template pipelines in this class instead of two pipelines shroing the same pipeline info. Decouple and use a different update info structure for each pipeline?




	ZoneScoped;
	if (chunkUpdateCount == 0)
		return;

	for (auto& i : descriptorManager.builderDescriptorSetsDetails)
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, i.set, 1, &descriptorManager.builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

	{
		TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting compute");
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, firstStagePipeline);
		commandBuffer.dispatch(chunkUpdateCount > maxChunkBaseLightingUpdatesPerFrame ? maxChunkBaseLightingUpdatesPerFrame : chunkUpdateCount, 1, 1);
	}

	// first stage writes light values to background layer which second stage reads from. Must synchronize accesss to this buffer.
	{
		vk::BufferMemoryBarrier barrier;
		barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = world->_worldMapBGDeviceBuffer;
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

	{
		TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting blur");
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, secondStagePipeline);
		commandBuffer.dispatch(chunkUpdateCount > maxChunkBaseLightingUpdatesPerFrame ? maxChunkBaseLightingUpdatesPerFrame : chunkUpdateCount, 1, 1);
	}
}