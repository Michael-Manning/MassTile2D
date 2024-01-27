#pragma once

#include <array>
#include <stdint.h>
#include <memory>

#include <vulkan/vulkan.hpp>
#include "VKEngine.h"
#include <glm/glm.hpp>


using dbVertexAtribute = std::array<vk::VertexInputAttributeDescription, 2>;

/// <summary>
/// Default vertex with 2D position and UVs
/// </summary>
struct Vertex {
	glm::vec2 pos;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingDescription;
	}

	static dbVertexAtribute getAttributeDescriptions() {
		dbVertexAtribute attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 2;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	static vk::PipelineVertexInputStateCreateInfo getVertexInputInfo(vk::VertexInputBindingDescription * bindingDescription, dbVertexAtribute * attribute) {

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		*bindingDescription = Vertex::getBindingDescription();
		*attribute = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>((*attribute).size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = (*attribute).data();
		return vertexInputInfo;
	}



};

static const std::vector<Vertex> quadVertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f}}
};

static const std::vector<uint16_t> QuadIndices = {
	0, 1, 2, 2, 3, 0
};

static void AllocateQuad(std::shared_ptr<VKEngine> engine, VertexMeshBuffer& vertexBuf) {
	{
		vk::DeviceSize bufferSize = sizeof(quadVertices[0]) * quadVertices.size();

		vk::Buffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		engine->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(engine->allocator, stagingBufferAllocation, &data);
		memcpy(data, quadVertices.data(), (size_t)bufferSize);
		vmaUnmapMemory(engine->allocator, stagingBufferAllocation);

		engine->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, 0, vertexBuf.vertexBuffer, vertexBuf.vertexBufferAllocation);

		engine->copyBuffer(stagingBuffer, vertexBuf.vertexBuffer, bufferSize);

		vkDestroyBuffer(engine->devContext.device, stagingBuffer, nullptr);
		vmaFreeMemory(engine->allocator, stagingBufferAllocation);
	}

	{
		vk::DeviceSize bufferSize = sizeof(QuadIndices[0]) * QuadIndices.size();

		vk::Buffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		engine->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(engine->allocator, stagingBufferAllocation, &data);
		memcpy(data, QuadIndices.data(), (size_t)bufferSize);
		vmaUnmapMemory(engine->allocator, stagingBufferAllocation);

		engine->createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, 0, vertexBuf.indexBuffer, vertexBuf.indexBufferAllocation);

		engine->copyBuffer(stagingBuffer, vertexBuf.indexBuffer, bufferSize);

		vkDestroyBuffer(engine->devContext.device, stagingBuffer, nullptr);
		vmaFreeMemory(engine->allocator, stagingBufferAllocation);
	}
}