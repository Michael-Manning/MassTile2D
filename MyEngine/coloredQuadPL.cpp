#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
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
#include "globalBufferDefinitions.h"
#include "Vertex.h"

#include "coloredQuadPL.h"

using namespace glm;
using namespace std;

//void ColoredQuadPL::CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc, MappedDoubleBuffer<void>& cameradb, bool flipFaces) {
void ColoredQuadPL::CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, MappedDoubleBuffer<void>& cameradb, bool flipFaces) {

	auto shaderStages = createShaderStages(vertexSrc, fragmentSrc);

	configureDescriptorSets(vector<Pipeline::descriptorSetInfo> {
		descriptorSetInfo(0, 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, &cameradb.buffers, cameradb.size),
		descriptorSetInfo(1, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, &ssboMappedDB.buffers, ssboMappedDB.size)
	});
	buildDescriptorLayouts();

	// create vector containing the builder descriptor set layouts
	vector< vk::DescriptorSetLayout> setLayouts;
	setLayouts.reserve(builderLayouts.size());
	for (auto& [set, layout] : builderLayouts)
		setLayouts.push_back(layout);
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
	pipelineInfo.renderPass = engine->renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	auto rv = engine->device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
	if (rv.result != vk::Result::eSuccess)
			throw std::runtime_error("failed to create graphics pipeline!");
	_pipeline = rv.value;

	for (auto& stage : shaderStages) {
		engine->device.destroyShaderModule(stage.module);
	}

	buildDescriptorSets();
}

void ColoredQuadPL::CreateInstancingBuffer() {
	engine->createMappedBuffer(sizeof(InstanceBufferData) * ColoredQuadPL_MAX_OBJECTS, vk::BufferUsageFlagBits::eStorageBuffer, ssboMappedDB);
}

void ColoredQuadPL::UploadInstanceData(std::vector<InstanceBufferData>& drawlist) {
	assert(drawlist.size() <= ColoredQuadPL_MAX_OBJECTS);
	memcpy(ssboMappedDB.buffersMapped[engine->currentFrame], drawlist.data(), sizeof(InstanceBufferData) * drawlist.size());
}

void ColoredQuadPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount) {

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

	for (auto& i : builderDescriptorSetsDetails)
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, i.set, 1, &builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

	{
		TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "Colored quad render");
		vkCmdDrawIndexed(commandBuffer, QuadIndices.size(), instanceCount, 0, 0, 0);
	}
}