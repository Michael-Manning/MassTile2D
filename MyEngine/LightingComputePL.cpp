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

void LightingComputePL::CreateComputePipeline(std::string computeSrc) {
	auto computeStage = createComputeShaderStage(computeSrc);

	std::array<VkBuffer, FRAMES_IN_FLIGHT> worldMapDeviceBuferRef = { world->_worldMapDeviceBuffer, world->_worldMapDeviceBuffer };
	configureDescriptorSets(vector<Pipeline::descriptorSetInfo> {
		descriptorSetInfo(0, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, &lightPositionsDB.buffers, sizeof(chunkLightingUpdateinfo)* (maxChunkUpdatesPerFrame)),
			descriptorSetInfo(1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, &worldMapDeviceBuferRef, sizeof(TileWorld::ssboObjectData)* (TileWorld_MAX_TILES))
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
	pipelineInfo.stage = computeStage;

	auto res = vkCreateComputePipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
	assert(res == VK_SUCCESS);

	buildDescriptorSets();
}

void LightingComputePL::createStagingBuffers() {
	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkUpdatesPerFrame, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, lightPositionsDB);
}

void LightingComputePL::recordCommandBuffer(VkCommandBuffer commandBuffer, int chunkUpdateCount) {
	if (chunkUpdateCount == 0)
		return;

	constexpr int workGroupSize = 32; // 32x32x1, same as tilemap chunk size

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);

	VkViewport viewport = fullframeViewport();
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	for (auto& i : builderDescriptorSetsDetails)
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, i.set, 1, &builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

	//int groupSize = (chunkUpdateCount * chunkSize) / workGroupSize;
	//vkCmdDispatch(commandBuffer, groupSize, groupSize, 1);

	vkCmdDispatch(commandBuffer, chunkUpdateCount > maxChunkUpdatesPerFrame ? maxChunkUpdatesPerFrame : chunkUpdateCount, 1, 1);

	//int groupSize = (chunkUpdateCount * chunkTileCount) / workGroupSize;
	//vkCmdDispatch(commandBuffer, chunkUpdateCount, 1, 1);

}