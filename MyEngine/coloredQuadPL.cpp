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

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>
#include <stb_image.h>

#include "VKengine.h"
#include "typedefs.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"
#include "Vertex.h"

#include "coloredQuadPL.h"

using namespace glm;
using namespace std;

namespace {
	struct pushConstant_s {
		glm::mat4 model;
		glm::vec4 color;
		int circle;
	};
}

void ColoredQuadPL::CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) {

	auto shaderStages = createShaderStages(vertexSrc, fragmentSrc);

	// setup vertex
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


	//setup push constants
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(pushConstant_s);
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;


	// setup layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(engine->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	auto inputAssembly = defaultInputAssembly();
	auto viewportState = defaultViewportState();
	auto rasterizer = defaultRasterizer();
	auto multisampling = defaultMultisampling();
	auto colorBlendAttachment = defaultColorBlendAttachment(true);
	auto colorBlending = defaultColorBlending(&colorBlendAttachment);
	auto dynamicState = defaultDynamicState();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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

	if (vkCreateGraphicsPipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	for (auto& stage : shaderStages) {
		vkDestroyShaderModule(engine->device, stage.module, nullptr);
	}
}

void ColoredQuadPL::createDescriptorSets(MappedDoubleBuffer& cameradb) {

	std::vector<VkDescriptorSetLayout> layouts(FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = engine->descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(engine->device, &allocInfo, generalDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	// one time update from UBO descriptor sets
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = cameradb.buffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(cameraUBO_s);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = generalDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void ColoredQuadPL::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(engine->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void ColoredQuadPL::createVertices() {
	{
		VkDeviceSize bufferSize = sizeof(quadVertices[0]) * quadVertices.size();

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(engine->allocator, stagingBufferAllocation, &data);
		memcpy(data, quadVertices.data(), (size_t)bufferSize);
		vmaUnmapMemory(engine->allocator, stagingBufferAllocation);

		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, vertexBuffer, vertexBufferAllocation);

		engine->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		vmaFreeMemory(engine->allocator, stagingBufferAllocation);
	}

	{
		VkDeviceSize bufferSize = sizeof(QuadIndices[0]) * QuadIndices.size();

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(engine->allocator, stagingBufferAllocation, &data);
		memcpy(data, QuadIndices.data(), (size_t)bufferSize);
		vmaUnmapMemory(engine->allocator, stagingBufferAllocation);

		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, indexBuffer, indexBufferAllocation);

		engine->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		vmaFreeMemory(engine->allocator, stagingBufferAllocation);
	}
}

void ColoredQuadPL::recordCommandBuffer(VkCommandBuffer commandBuffer, std::vector<DrawItem>& drawlist) {

	if (drawlist.size() == 0) 
		return;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)engine->swapChainExtent.width;
	viewport.height = (float)engine->swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = engine->swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &generalDescriptorSets[engine->currentFrame], 0, nullptr);

	for (auto& draw : drawlist)
	{
		mat4 model = translate(mat4(1.0), vec3(draw.position.x, draw.position.y, 0.0));
		model = rotate(model, draw.rotation, vec3(0.0f, 0.0f, 1.0f));
		model = scale(model, vec3(draw.scale.x, draw.scale.y, 0.0));

		mat4 nextmodel = mat4(1.0);
		pushConstant_s pushData{
			model,
			draw.color,
			draw.circle ? 1 : 0
		};
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstant_s), &pushData);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(QuadIndices.size()), 1, 0, 0, 0);
	}
}
