#include "stdafx.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <fstream>
#include <chrono>
#include <memory>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <stb_image.h>

#include "VKengine.h"
#include "typedefs.h"
#include "vulkan_util.h"
#include "texturedQuadPL.h"
#include "globalBufferDefinitions.h"
#include "Vertex.h"
#include "GlobalImageDescriptor.h"

using namespace glm;
using namespace std;


void TexturedQuadPL::CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, GlobalImageDescriptor* textureDescriptor, MappedDoubleBuffer<cameraUBO_s>& cameradb, std::array<int, 2>& lightMapTextureIndexes, bool flipFaces) {

	this->textureDescriptor = textureDescriptor;

	engine->createMappedBuffer(sizeof(ssboObjectInstanceData) * TexturedQuadPL_MAX_OBJECTS, vk::BufferUsageFlagBits::eStorageBuffer, ssboMappedDB);

	engine->createMappedBuffer(sizeof(lightMapIndex_UBO), vk::BufferUsageFlagBits::eUniformBuffer, lightmapIndexDB);

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		lightmapIndexDB.buffersMapped[i]->lightMapIndex = lightMapTextureIndexes[i];

	auto shaderStages = createGraphicsShaderStages(vertexSrc, fragmentSrc);

	descriptorManager.configureDescriptorSets(vector<DescriptorManager::descriptorSetInfo> {
		DescriptorManager::descriptorSetInfo(1, 1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, &cameradb.buffers, cameradb.size),
		DescriptorManager::descriptorSetInfo(1, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, &ssboMappedDB.buffers, ssboMappedDB.size),
		DescriptorManager::descriptorSetInfo(1, 2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment, &lightmapIndexDB.buffers, lightmapIndexDB.size)
	});
	descriptorManager.buildDescriptorLayouts();

	descriptorLayoutMap setLayouts;

	for (auto& [set, layout] : descriptorManager.builderLayouts)
		setLayouts[set] = layout;
	setLayouts[0] = textureDescriptor->layout;

	buildPipelineLayout(setLayouts);

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

	if (flipFaces)
		rasterizer.frontFace = vk::FrontFace::eClockwise;

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

void TexturedQuadPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount){
	if (instanceCount == 0)
		return;

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &textureDescriptor->descriptorSets[engine->currentFrame], 0, VK_NULL_HANDLE);
	}

	for (auto& i : descriptorManager.builderDescriptorSetsDetails)
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, i.set, 1, &descriptorManager.builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

	{
		TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "textured quad render");
		commandBuffer.drawIndexed(QuadIndices.size(), instanceCount, 0, 0, 0);
	}
}
