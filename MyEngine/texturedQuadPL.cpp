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
#include "pipelines.h"
#include "typedefs.h"

#include "vulkan_util.h"

using namespace glm;
using namespace std;

namespace {

	struct pushConstant_s {
		alignas(8) vec2 cameraTranslation;
		float cameraZoom;
		alignas(8) vec2 translation;
		alignas(8) vec2 scale;
		float rotation = 0.0f;
		int index;
		int32_t padding[2];
	};

	struct UBO_s {
		float aspectRatio = 1.0f;
	};


	struct Vertex {
		glm::vec2 pos;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 2;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}
	};


	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};
}

void TexturedQuadPL::CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) {

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

void TexturedQuadPL::createDescriptorSets() {

	assert(defaultTexture.textureImageAllocation != nullptr);

	std::vector<VkDescriptorSetLayout> layouts(FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = engine->descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	//generalDescriptorSets.resize(FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(engine->device, &allocInfo, generalDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	// one time update from UBO descriptor sets
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBO_s);

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

	// all descriptor sets must recieve one update before use. Fill image descriptor with default texture
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		std::vector<VkDescriptorImageInfo> imageInfos(TexturedQuadPL_MAX_TEXTURES);

		for (size_t j = 0; j < TexturedQuadPL_MAX_TEXTURES; j++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = defaultTexture.imageView;
			imageInfo.sampler = defaultTexture.sampler;

			imageInfos[j] = imageInfo;
		}


		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = generalDescriptorSets[i];
		descriptorWrites[0].dstBinding = 1;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = TexturedQuadPL_MAX_TEXTURES;
		descriptorWrites[0].pImageInfo = imageInfos.data();

		vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void TexturedQuadPL::updateUBO(float aspect) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UBO_s ubo;
	ubo.aspectRatio = aspect;

	memcpy(uniformBuffersMapped[engine->currentFrame], &ubo, sizeof(ubo));
}

// For now, update every descriptor array element. Could update individual elements as an optimization
void TexturedQuadPL::updateDescriptorSets() {

	if (descriptorDirtyFlags[engine->currentFrame] == false)
		return;

	std::vector<VkDescriptorImageInfo> imageInfos(TexturedQuadPL_MAX_TEXTURES);

	bindIndexes[engine->currentFrame].clear();

	for (int i = 0; i < TexturedQuadPL_MAX_TEXTURES; i++)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (textures[i].first == -1) {
			imageInfo.imageView = defaultTexture.imageView;
			imageInfo.sampler = defaultTexture.sampler;
		}
		else {
			imageInfo.imageView = textures[i].second.imageView;
			imageInfo.sampler = textures[i].second.sampler;
		}

		imageInfos[i] = imageInfo;

		bindIndexes[engine->currentFrame][textures[i].first] = i;
	}


	std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = generalDescriptorSets[engine->currentFrame];
	descriptorWrites[0].dstBinding = 1;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = TexturedQuadPL_MAX_TEXTURES;
	descriptorWrites[0].pImageInfo = imageInfos.data();

	vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	descriptorDirtyFlags[engine->currentFrame] = false;
}


void TexturedQuadPL::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = TexturedQuadPL_MAX_TEXTURES;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(engine->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void TexturedQuadPL::createUniformBuffers() {

	VkDeviceSize bufferSize = sizeof(UBO_s);

	uniformBuffers.resize(FRAMES_IN_FLIGHT);
	uniformBuffersAllocation.resize(FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, uniformBuffers[i], uniformBuffersAllocation[i]);
		vmaMapMemory(engine->allocator, uniformBuffersAllocation[i], &uniformBuffersMapped[i]);
	}
}

void TexturedQuadPL::createVertices() {
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(engine->allocator, stagingBufferAllocation, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vmaUnmapMemory(engine->allocator, stagingBufferAllocation);

		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, vertexBuffer, vertexBufferAllocation);

		engine->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		vmaFreeMemory(engine->allocator, stagingBufferAllocation);
	}

	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(engine->allocator, stagingBufferAllocation, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vmaUnmapMemory(engine->allocator, stagingBufferAllocation);

		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, indexBuffer, indexBufferAllocation);

		engine->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
		vmaFreeMemory(engine->allocator, stagingBufferAllocation);
	}
}

void TexturedQuadPL::recordCommandBuffer(VkCommandBuffer commandBuffer, std::vector<DrawItem>& drawlist) {

	if (drawlist.size() == 0) {
		//return;
	}

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

	

	for (int i = 0; i < drawlist.size(); i++)
	{
		auto& draw = drawlist[i];

		pushConstant_s o;
		o.cameraTranslation = cameraPosition;
		o.cameraZoom = cameraZoom;
		o.translation = draw.position;
		o.scale = draw.scale;
		o.rotation = draw.rotation;
		o.index = bindIndexes[engine->currentFrame][draw.texture];

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstant_s), &o);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	}


	//for (auto& draw : drawlist)
	//{
	//	mat4 view = mat4(1.0);
	//	view = scale(view, vec3(cameraZoom));
	//	view = translate(view, vec3(-cameraPosition.x, cameraPosition.y, 0.0));
	//	view = scale(view, vec3(1.0, -1.0, 1.0)); // make up the positive direction

	//	mat4 model = translate(mat4(1.0), vec3(draw.position.x, draw.position.y, 0.0));
	//	if(draw.rotation != 0.0f)
	//		model = rotate(model, draw.rotation, vec3(0.0f, 0.0f, 1.0f));
	//	model = scale(model, vec3(draw.scale.x, draw.scale.y, 0.0));

	//	pushConstant_s pushData{
	//		model,
	//		view,
	//		bindIndexes[engine->currentFrame][draw.texture]
	//	};
	//	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstant_s), &pushData);

	//	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	//}
}


void TexturedQuadPL::addTextureBinding(texID ID, Texture texture) {
	// map a binding in the next availble slot of the vector

	// attempting to bind duplicate ID is acceptable, but redundant
	if (boundIDs.contains(ID)) {
		return;
	}

	assert(bindingCount < TexturedQuadPL_MAX_TEXTURES);

	for (size_t i = 0; i < TexturedQuadPL_MAX_TEXTURES; i++)
	{
		// available slot
		if (textures[i].first == -1) {
			textures[i] = pair(ID, texture);
			goto found;
		}
	}

	assert(false); // somehow did not find available spot

found:;
	bindingCount++;
	boundIDs.insert(ID);

	// both descriptor sets are now out of date
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		descriptorDirtyFlags[i] = true;
	}
}

void TexturedQuadPL::removeTextureBinding(texID ID) {

	assert(boundIDs.contains(ID)); // missuse

	for (size_t i = 0; i < TexturedQuadPL_MAX_TEXTURES; i++)
	{
		if (textures[i].first == ID) {
			// mark as available
			textures[i] = pair<texID, Texture>(-1, { 0 });
		}
	}
	boundIDs.erase(ID);
}