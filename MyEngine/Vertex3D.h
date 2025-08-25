#pragma once

#include <array>
#include <stdint.h>
#include <memory>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "Vertex2D.h" // for dbVertexAtribute

struct Vertex3D {
	glm::vec3 pos;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex3D);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingDescription;
	}

	static dbVertexAtribute getAttributeDescriptions() {
		dbVertexAtribute attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex3D, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 2;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex3D, texCoord);

		return attributeDescriptions;
	}

	static vk::PipelineVertexInputStateCreateInfo getVertexInputInfo(vk::VertexInputBindingDescription* bindingDescription, dbVertexAtribute* attribute) {

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		*bindingDescription = Vertex3D::getBindingDescription();
		*attribute = Vertex3D::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>((*attribute).size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = (*attribute).data();
		return vertexInputInfo;
	}
};