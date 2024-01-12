#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <string>
#include <utility>

#include <vulkan/vulkan.hpp>
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

	void CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, MappedDoubleBuffer<void>& cameradb);
	//void CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc, MappedDoubleBuffer<void>& cameradb);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer);

	void setTextureAtlas(Texture* textureAtlas) {

		// fill in missing data of descriptor set builder before submitting

		this->textureAtlas = textureAtlas;
		builderDescriptorSetsDetails[1].textures = this->textureAtlas.value();

		std::array<vk::Buffer, FRAMES_IN_FLIGHT> worldMapDeviceBuferRef = { world->_worldMapFGDeviceBuffer, world->_worldMapFGDeviceBuffer };
		builderDescriptorSetsDetails[2].doubleBuffer = &worldMapDeviceBuferRef;

		std::array<vk::Buffer, FRAMES_IN_FLIGHT> worldMapBGDeviceBuferRef = { world->_worldMapBGDeviceBuffer, world->_worldMapBGDeviceBuffer };
		builderDescriptorSetsDetails[3].doubleBuffer = &worldMapBGDeviceBuferRef;

		buildDescriptorSets();

	}

	std::optional<Texture*> textureAtlas;
private:


	VKUtil::BufferUploader<cameraUBO_s> cameraUploader;

	std::shared_ptr<TileWorld> world = nullptr;

	std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorSetLayout SSBOSetLayout;
};
