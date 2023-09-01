#pragma once

#include <array>
#include <stdint.h>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

/// <summary>
/// Default vertex with 2D position and UVs
/// </summary>
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

static const std::vector<Vertex> quadVertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f}}
};

static const std::vector<uint16_t> QuadIndices = {
	0, 1, 2, 2, 3, 0
};