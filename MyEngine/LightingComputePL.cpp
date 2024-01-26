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

	std::array<vk::Buffer, FRAMES_IN_FLIGHT> worldMapFGDeviceBuferRef = { world->_worldMapFGDeviceBuffer, world->_worldMapFGDeviceBuffer };
	std::array<vk::Buffer, FRAMES_IN_FLIGHT> worldMapBGDeviceBuferRef = { world->_worldMapBGDeviceBuffer, world->_worldMapBGDeviceBuffer };
	descriptorManager.configureDescriptorSets(vector<DescriptorManager::descriptorSetInfo> {
		DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &lightPositionsDB.buffers, sizeof(chunkLightingUpdateinfo)* (maxChunkUpdatesPerFrame)),
		DescriptorManager::descriptorSetInfo(1, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapFGDeviceBuferRef, sizeof(TileWorld::ssboObjectData)* (mapCount)),
		DescriptorManager::descriptorSetInfo(1, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute, &worldMapBGDeviceBuferRef, sizeof(TileWorld::ssboObjectData)* (mapCount))
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

void LightingComputePL::createStagingBuffers() {
	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, lightPositionsDB);
}

void LightingComputePL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int chunkUpdateCount) {
	ZoneScoped;
	if (chunkUpdateCount == 0)
		return;

	{
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, firstStagePipeline);

		for (auto& i : descriptorManager.builderDescriptorSetsDetails)
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, i.set, 1, &descriptorManager.builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

		{
			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting compute");
			commandBuffer.dispatch(chunkUpdateCount > maxChunkUpdatesPerFrame ? maxChunkUpdatesPerFrame : chunkUpdateCount, 1, 1);
		}
	}

	// idk if you have to rebind the same desciptor sets just because you bind a different pipeline. Do you need to unbind descriptor sets?
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, secondStagePipeline);

		for (auto& i : descriptorManager.builderDescriptorSetsDetails)
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, i.set, 1, &descriptorManager.builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

		{
			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting blur");
			commandBuffer.dispatch(chunkUpdateCount > maxChunkUpdatesPerFrame ? maxChunkUpdatesPerFrame : chunkUpdateCount, 1, 1);
		}
	}
}