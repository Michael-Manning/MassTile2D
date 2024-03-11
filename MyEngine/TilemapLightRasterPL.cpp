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

#include "TilemapLightRasterPL.h"

using namespace glm;
using namespace std;

namespace {
	//struct pushConstant_s {
	//	int32_t textureIndex;
	//};
}

void TilemapLightRasterPL::CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, GlobalImageDescriptor* textureDescriptor, MappedDoubleBuffer<cameraUBO_s>& cameradb) {

	this->textureDescriptor = textureDescriptor;

	auto shaderStages = createGraphicsShaderStages(vertexSrc, fragmentSrc);

	auto worldMapFGDeviceBuferRef = world->MapFGBuffer.GetDoubleBuffer();
	auto worldMapBGDeviceBuferRef = world->MapBGBuffer.GetDoubleBuffer();
	auto  worldMapUpscaleBuferRef = world->MapLightUpscaleBuffer.GetDoubleBuffer();
	auto  worldMapBlurBuferRef = world->MapLightBlurBuffer.GetDoubleBuffer();

	descriptorManager.configureDescriptorSets(vector<DescriptorManager::descriptorSetInfo> {
		DescriptorManager::descriptorSetInfo(1, 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, &cameradb.buffers, cameradb.size),
		DescriptorManager::descriptorSetInfo(1, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, &worldMapFGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo)* (mapCount)),
		DescriptorManager::descriptorSetInfo(1, 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, &worldMapBGDeviceBuferRef, sizeof(TileWorld::worldTile_ssbo)* (mapCount)),
		DescriptorManager::descriptorSetInfo(1, 3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, &worldMapUpscaleBuferRef, world->MapLightUpscaleBuffer.size),
		DescriptorManager::descriptorSetInfo(1, 4, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, &worldMapBlurBuferRef, world->MapLightBlurBuffer.size)
	});
	descriptorManager.buildDescriptorLayouts();

	descriptorLayoutMap setLayouts;
	for (auto& [set, layout] : descriptorManager.builderLayouts)
		setLayouts[set] = layout;
	setLayouts[0] = textureDescriptor->layout;

	buildPipelineLayout(setLayouts, sizeof(TileWorld::lightingSettings_pc), vk::ShaderStageFlagBits::eFragment);
	//buildPipelineLayout(setLayouts, sizeof(pushConstant_s), vk::ShaderStageFlagBits::eFragment);
	//buildPipelineLayout(setLayouts);

	vk::VertexInputBindingDescription VbindingDescription;
	dbVertexAtribute Vattribute;
	auto vertexInputInfo = Vertex::getVertexInputInfo(&VbindingDescription, &Vattribute);
	auto inputAssembly = defaultInputAssembly();
	auto viewportState = defaultViewportState();
	auto rasterizer = defaultRasterizer();
	auto multisampling = defaultMultisampling();
	/*auto colorBlendAttachment = defaultColorBlendAttachment(true);*/
	auto dynamicState = defaultDynamicState();


	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eR;
	//colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eG;
	//colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eB;
	//colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eA;



	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

	auto colorBlending = defaultColorBlending(&colorBlendAttachment);


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

void TilemapLightRasterPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex) {

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);


	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &textureDescriptor->descriptorSets[engine->currentFrame], 0, VK_NULL_HANDLE);
	}

	for (auto& i : descriptorManager.builderDescriptorSetsDetails)
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, i.set, 1, &descriptorManager.builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

	//pushConstant_s pc{ .textureIndex = textureIndex };
	//commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(pushConstant_s), &pc);
	commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(TileWorld::lightingSettings_pc), &world->lightingSettings);

	{
		TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "Tilemap lighting raster");
		commandBuffer.drawIndexed(QuadIndices.size(), 1, 0, 0, 0);
	}
}