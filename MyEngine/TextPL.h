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

	void CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc, MappedDoubleBuffer& cameradb);
	void recordCommandBuffer(VkCommandBuffer commandBuffer);



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
