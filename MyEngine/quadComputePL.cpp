
#include "stdafx.h"

// this ended up being slower than just calculating all four vertices in the vertex shaders
#if false

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

#include "quadComputePL.h"

using namespace glm;
using namespace std;


void QuadComputePL::allocateTransformBuffer(VkBuffer& buffer, VmaAllocation& allocation) {
	// device only buffer. Does not need to allow copying via staging buffer as it is only written to by compute shader
	engine->createBuffer(sizeof(transformSSBO_430) * (maxInstanceCount), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, buffer, allocation, true);
}

void QuadComputePL::CreateStagingBuffers() {
	engine->createMappedBuffer(sizeof(InstanceBufferData) * maxInstanceCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, ssboMappedDB);
}

void QuadComputePL::UploadInstanceData(std::vector<InstanceBufferData>& drawlist) {
	assert(drawlist.size() <= maxInstanceCount);
	memcpy(ssboMappedDB.buffersMapped[engine->currentFrame], drawlist.data(), sizeof(InstanceBufferData) * drawlist.size());
}

void QuadComputePL::CreateComputePipeline(const std::vector<uint8_t>& computeSrc, MappedDoubleBuffer<>& cameradb, VkBuffer transformBuffer) {
	auto computeStage = createComputeShaderStage(computeSrc);

	std::array<VkBuffer, FRAMES_IN_FLIGHT> transformBufferRef = { transformBuffer, transformBuffer };
	configureDescriptorSets(vector<Pipeline::descriptorSetInfo> {
		descriptorSetInfo(0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, &cameradb.buffers, cameradb.size),
		descriptorSetInfo(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, &ssboMappedDB.buffers, sizeof(InstanceBufferData)* (maxInstanceCount)),
			descriptorSetInfo(1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, &transformBufferRef, sizeof(transformSSBO_430)* (maxInstanceCount)),
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

void QuadComputePL::recordCommandBuffer(VkCommandBuffer commandBuffer, int instanceCount) {
	ZoneScoped;
	if (instanceCount == 0)
		return;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);

	VkViewport viewport = fullframeViewport();
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	for (auto& i : builderDescriptorSetsDetails)
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, i.set, 1, &builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

	constexpr int workGroupSize = 256;

	{
		TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Transform compute");
		vkCmdDispatch(commandBuffer, instanceCount / 256 + (instanceCount % 256 ? 1 : 0), 1, 1);
	}

}

#endif