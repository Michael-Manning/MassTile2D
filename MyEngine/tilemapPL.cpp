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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VKengine.h"
#include "typedefs.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"
#include "Vertex.h"

#include "tilemapPL.h"

using namespace glm;
using namespace std;

namespace {
	struct pushConstant_s{
		int32_t textureIndex;
		int32_t lightMapIndex;
	};
}

void TilemapPL::CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, GlobalImageDescriptor* textureDescriptor, MappedDoubleBuffer<cameraUBO_s>& cameradb) {

	this->textureDescriptor = textureDescriptor;

	auto shaderStages = createGraphicsShaderStages(vertexSrc, fragmentSrc);

	auto worldMapFGDeviceBuferRef = world->MapFGBuffer.GetDoubleBuffer();
	auto worldMapBGDeviceBuferRef = world->MapBGBuffer.GetDoubleBuffer();

	descriptorManager.configureDescriptorSets(vector<DescriptorManager::descriptorSetInfo> {
		DescriptorManager::descriptorSetInfo(1, 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, &cameradb.buffers, cameradb.size),
		DescriptorManager::descriptorSetInfo(1, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, &worldMapFGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo)* (mapCount)),
		DescriptorManager::descriptorSetInfo(1, 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, &worldMapBGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo)* (mapCount))
	});
	descriptorManager.buildDescriptorLayouts();

	descriptorLayoutMap setLayouts;
	for (auto& [set, layout] : descriptorManager.builderLayouts)
		setLayouts[set] = layout;
	setLayouts[0] = textureDescriptor->layout;

	buildPipelineLayout(setLayouts, sizeof(pushConstant_s), vk::ShaderStageFlagBits::eFragment);

	vk::VertexInputBindingDescription VbindingDescription;
	dbVertexAtribute Vattribute;
	auto vertexInputInfo = Vertex::getVertexInputInfo(&VbindingDescription, &Vattribute);
	auto inputAssembly = defaultInputAssembly();
	auto viewportState = defaultViewportState();
	auto rasterizer = defaultRasterizer();
	auto multisampling = defaultMultisampling();
	auto colorBlendAttachment = defaultColorBlendAttachment(true);
	auto colorBlending = defaultColorBlending(&colorBlendAttachment);
	auto dynamicState = defaultDynamicState();

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderTarget;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	auto rv = engine->devContext.device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
	if (rv.result != vk::Result::eSuccess)
		throw std::runtime_error("failed to create graphics pipeline!");
	_pipeline = rv.value;

	for (auto& stage : shaderStages) {
		engine->devContext.device.destroyShaderModule(stage.module);
	}

	descriptorManager.buildDescriptorSets();
}

void TilemapPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex, int lightMapIndex) {

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);


	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &textureDescriptor->descriptorSets[engine->currentFrame], 0, VK_NULL_HANDLE);
	}

	for (auto& i : descriptorManager.builderDescriptorSetsDetails)
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, i.set, 1, &descriptorManager.builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

	pushConstant_s pc{
		.textureIndex = textureIndex,
		.lightMapIndex = lightMapIndex
	};
	commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(pushConstant_s), &pc);

	{
		TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "Tilemap render");
		commandBuffer.drawIndexed(QuadIndices.size(), 1, 0, 0, 0);
	}
}