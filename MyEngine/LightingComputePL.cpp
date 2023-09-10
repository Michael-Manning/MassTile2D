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

#include <vulkan/vulkan.h>
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

void LightingComputePL::CreateComputePipeline(std::string computeSrc_firstPass, std::string computeSrc_secondPass) {
	auto firstComputeStage = createComputeShaderStage(computeSrc_firstPass);
	auto secondComputeStage = createComputeShaderStage(computeSrc_secondPass);

	std::array<VkBuffer, FRAMES_IN_FLIGHT> worldMapFGDeviceBuferRef = { world->_worldMapFGDeviceBuffer, world->_worldMapFGDeviceBuffer };
	std::array<VkBuffer, FRAMES_IN_FLIGHT> worldMapBGDeviceBuferRef = { world->_worldMapBGDeviceBuffer, world->_worldMapBGDeviceBuffer };
	configureDescriptorSets(vector<Pipeline::descriptorSetInfo> {
		descriptorSetInfo(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, &lightPositionsDB.buffers, sizeof(chunkLightingUpdateinfo)* (maxChunkUpdatesPerFrame)),
		descriptorSetInfo(1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, &worldMapFGDeviceBuferRef, sizeof(TileWorld::ssboObjectData)* (TileWorld_MAX_TILES)),
		descriptorSetInfo(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, &worldMapBGDeviceBuferRef, sizeof(TileWorld::ssboObjectData)* (TileWorld_MAX_TILES))
	});
	buildDescriptorLayouts();

	// create vector containing the builder descriptor set layouts
	vector< VkDescriptorSetLayout> setLayouts;
	setLayouts.reserve(builderLayouts.size());
	for (auto& [set, layout] : builderLayouts) {
		setLayouts.push_back(layout);
	}
	buildPipelineLayout(setLayouts);

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = pipelineLayout;

	{
		pipelineInfo.stage = firstComputeStage;
		auto res = vkCreateComputePipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &firstStagePipeline);
		assert(res == VK_SUCCESS);
	}
	{
		pipelineInfo.stage = secondComputeStage;
		auto res = vkCreateComputePipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &secondStagePipeline);
		assert(res == VK_SUCCESS);
	}

	buildDescriptorSets();
}

void LightingComputePL::createStagingBuffers() {
	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkUpdatesPerFrame, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, lightPositionsDB);
}

void LightingComputePL::recordCommandBuffer(VkCommandBuffer commandBuffer, int chunkUpdateCount) {
	ZoneScoped;
	if (chunkUpdateCount == 0)
		return;

	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, firstStagePipeline);

		for (auto& i : builderDescriptorSetsDetails)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, i.set, 1, &builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

		{
			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting compute");
			vkCmdDispatch(commandBuffer, chunkUpdateCount > maxChunkUpdatesPerFrame ? maxChunkUpdatesPerFrame : chunkUpdateCount, 1, 1);
		}
	}
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, secondStagePipeline);

		for (auto& i : builderDescriptorSetsDetails)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, i.set, 1, &builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

		{
			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting blur");
			vkCmdDispatch(commandBuffer, chunkUpdateCount > maxChunkUpdatesPerFrame ? maxChunkUpdatesPerFrame : chunkUpdateCount, 1, 1);
		}
	}
}