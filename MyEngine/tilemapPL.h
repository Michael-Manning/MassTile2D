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
#include "TileWorld.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"

class TilemapPL :public  Pipeline {
public:

	TilemapPL(std::shared_ptr<VKEngine>& engine, std::shared_ptr<TileWorld> world) : Pipeline(engine), world(world) {
	}

	void CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc, MappedDoubleBuffer<void>& cameradb);
	void recordCommandBuffer(VkCommandBuffer commandBuffer);

	void setTextureAtlas(Texture textureAtlas) {

		// fill in missing data of descriptor set builder before submitting

		this->textureAtlas = textureAtlas;
		builderDescriptorSetsDetails[1].textures = &this->textureAtlas.value();

		std::array<VkBuffer, FRAMES_IN_FLIGHT> worldMapDeviceBuferRef = { world->_worldMapFGDeviceBuffer, world->_worldMapFGDeviceBuffer };
		builderDescriptorSetsDetails[2].doubleBuffer = &worldMapDeviceBuferRef;

		std::array<VkBuffer, FRAMES_IN_FLIGHT> worldMapBGDeviceBuferRef = { world->_worldMapBGDeviceBuffer, world->_worldMapBGDeviceBuffer };
		builderDescriptorSetsDetails[3].doubleBuffer = &worldMapBGDeviceBuferRef;

		buildDescriptorSets();

	}

	std::optional<Texture> textureAtlas;
private:


	VKUtil::BufferUploader<cameraUBO_s> cameraUploader;

	std::shared_ptr<TileWorld> world = nullptr;

	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout SSBOSetLayout;
};
