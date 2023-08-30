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

#include <omp.h>

#include "VKengine.h"
#include "pipelines.h"
#include "typedefs.h"

#include "vulkan_util.h"

using namespace glm;
using namespace std;

namespace {

	struct pushConstant_s {
		vec2 cameraTranslation;
		float cameraZoom;
		int mapw;
		int maph;
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

void TilemapPL::CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) {

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

	vector< VkDescriptorSetLayout> setLayouts = { descriptorSetLayout, SSBOSetLayout };
	buildPipelineLayout(setLayouts, sizeof(pushConstant_s), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

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

void TilemapPL::createDescriptorSets() {

	assert(textureAtlas.has_value());

	// general descriptor sets
	{
		buidDBDescriptorSet(descriptorSetLayout, generalDescriptorSets);

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

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureAtlas.value().imageView;
			imageInfo.sampler = textureAtlas.value().sampler;

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = generalDescriptorSets[i];
			descriptorWrites[0].dstBinding = 1;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

	}

	// SSBO descriptor sets
	{
		buidDBDescriptorSet(SSBOSetLayout, ssboDescriptorSets);

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {

			VkDescriptorBufferInfo objectBufferInfo;
			objectBufferInfo.buffer = worldMapDeviceBuffer;
			//objectBufferInfo.buffer = ssboMappedDB.buffers[i];
			objectBufferInfo.offset = 0;
			objectBufferInfo.range = sizeof(ssboObjectData) * (TilemapPL_MAX_TILES);

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = ssboDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &objectBufferInfo;


			vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		}
	}
}

void TilemapPL::updateUBO(float aspect) {
	uboData.aspectRatio = aspect;
	invalidateAspectUBO();
}


void TilemapPL::createDescriptorSetLayout() {

	// general set layout
	vector<VkDescriptorSetLayoutBinding> set0Bindings = {
		buildUBOBinding(0, VK_SHADER_STAGE_VERTEX_BIT),
		buildSamplerBinding(1, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	buildSetLayout(set0Bindings, descriptorSetLayout);

	// ssbo set layout
	vector<VkDescriptorSetLayoutBinding> set1Bindings = {
		buildSSBOBinding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	buildSetLayout(set1Bindings, SSBOSetLayout);
}

void TilemapPL::createUniformBuffers() {

	VkDeviceSize bufferSize = sizeof(UBO_s);

	uniformBuffers.resize(FRAMES_IN_FLIGHT);
	uniformBuffersAllocation.resize(FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		engine->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, uniformBuffers[i], uniformBuffersAllocation[i]);
		vmaMapMemory(engine->allocator, uniformBuffersAllocation[i], &uniformBuffersMapped[i]);
	}
}

void TilemapPL::createSSBOBuffer() {

	//engine->createMappedBuffer(sizeof(ssboObjectData) * (TilemapPL_MAX_TILES), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, ssboMappedDB);
}

void TilemapPL::createVertices() {
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

void TilemapPL::createWorldBuffer() {
		engine->createBuffer(sizeof(ssboObjectData) * (TilemapPL_MAX_TILES), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, worldMapDeviceBuffer, worldMapDeviceBufferAllocation, true);
}
void TilemapPL::createChunkTransferBuffers() {
	// create large buffer for initial upload
	engine->createBuffer(sizeof(ssboObjectData) * (largeChunkCount), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, largeChunkBuffer, largeChunkAllocation);
	vmaMapMemory(engine->allocator, largeChunkAllocation, &largeChunkBufferMapped);

	// create small buffers for individual chunk updates
	engine->createMappedBuffer(sizeof(ssboObjectData) * chunkCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, chunkTransferBuffers);
}

void TilemapPL::recordCommandBuffer(VkCommandBuffer commandBuffer) {



	if (uboDirtyFlags[engine->currentFrame] == true) {
		memcpy(uniformBuffersMapped[engine->currentFrame], &uboData, sizeof(uboData));
		uboDirtyFlags[engine->currentFrame] = false;
	}


	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport = fullframeViewport();
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
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &ssboDescriptorSets[engine->currentFrame], 0, nullptr);

	pushConstant_s pushData{
		camera.position,
		camera.zoom,
		mapW,
		mapH
	};

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstant_s), &pushData);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void TilemapPL::stageChunkUpdates(VkCommandBuffer commandBuffer) {

	// ignoring chunk update range optimization for first test


	for (size_t i = 0; i < chunkCount; i++){
		if (chunkDirtyFlags[i] == true) {
			memcpy(chunkTransferBuffers.buffersMapped[engine->currentFrame], mapData.data() + i * chunkTileCount, sizeof(ssboObjectData) * chunkTileCount);
			chunkDirtyFlags[i] = false;
	
			{
				VkBufferCopy copyRegion{};
				copyRegion.size = chunkTileCount * sizeof(uboData);
				copyRegion.dstOffset = i * chunkTileCount * sizeof(ssboObjectData);
				copyRegion.srcOffset = 0;

				// expand by increasing size of transfer buffer. Upload multiple chunks by specifying multiple copy regions
				vkCmdCopyBuffer(commandBuffer, chunkTransferBuffers.buffers[engine->currentFrame], worldMapDeviceBuffer, 1, &copyRegion);
			}
		}
	}
}