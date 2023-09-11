#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <string>
#include <utility>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "vertex.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"


constexpr int TEXTPL_maxTextObjects = 10;
constexpr int TEXTPL_maxTextLength = 128;

class TextPL :public  Pipeline {
public:

	TextPL(std::shared_ptr<VKEngine>& engine) : Pipeline(engine) {
	}

	struct charQuad {
		glm::vec2 uvmin;
		glm::vec2 uvmax;
		glm::vec2 scale;
		glm::vec2 position;

	};


	void CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc, MappedDoubleBuffer& cameradb);
	void recordCommandBuffer(VkCommandBuffer commandBuffer);

	void setTextureAtlas(Texture textureAtlas) {

		// fill in missing data of descriptor set builder before submitting

		this->textureAtlas = textureAtlas;
		builderDescriptorSetsDetails[1].texture = &this->textureAtlas.value();

		std::array<VkBuffer, FRAMES_IN_FLIGHT> worldMapDeviceBuferRef = { world->_worldMapFGDeviceBuffer, world->_worldMapFGDeviceBuffer };
		builderDescriptorSetsDetails[2].doubleBuffer = &worldMapDeviceBuferRef;

		std::array<VkBuffer, FRAMES_IN_FLIGHT> worldMapBGDeviceBuferRef = { world->_worldMapBGDeviceBuffer, world->_worldMapBGDeviceBuffer };
		builderDescriptorSetsDetails[3].doubleBuffer = &worldMapBGDeviceBuferRef;

		buildDescriptorSets();

	}

	std::optional<Texture> textureAtlas;
private:

	struct textObject {
		charQuad quads[TEXTPL_maxTextLength];
	};

	textObject textData[TEXTPL_maxTextObjects];
	int textLengths[TEXTPL_maxTextObjects];

	struct textIndexes_ubo {
		int indexes[TEXTPL_maxTextObjects];
	};

	VKUtil::UBOUploader<cameraUBO_s> cameraUploader;


	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout SSBOSetLayout;
};
