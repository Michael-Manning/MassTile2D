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

	TilemapPL(std::shared_ptr<VKEngine>& engine, TileWorld* world) : Pipeline(engine), world(world) {
	}

	void CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, GlobalImageDescriptor* textureDescriptor, MappedDoubleBuffer<cameraUBO_s>& cameradb);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex);

	//void setTextureAtlas(Texture* textureAtlas) {

	//	// fill in missing data of descriptor set builder before submitting

	//	this->textureAtlas = textureAtlas;
	//	descriptorManager.builderDescriptorSetsDetails[1].textures = this->textureAtlas.value();

	//	std::array<vk::Buffer, FRAMES_IN_FLIGHT> worldMapDeviceBuferRef = { world->_worldMapFGDeviceBuffer, world->_worldMapFGDeviceBuffer };
	//	descriptorManager.builderDescriptorSetsDetails[2].doubleBuffer = &worldMapDeviceBuferRef;

	//	std::array<vk::Buffer, FRAMES_IN_FLIGHT> worldMapBGDeviceBuferRef = { world->_worldMapBGDeviceBuffer, world->_worldMapBGDeviceBuffer };
	//	descriptorManager.builderDescriptorSetsDetails[3].doubleBuffer = &worldMapBGDeviceBuferRef;

	//	descriptorManager.buildDescriptorSets();

	//}

	//std::optional<Texture*> textureAtlas;
private:

	TileWorld* world = nullptr;

	GlobalImageDescriptor* textureDescriptor;

	std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorSetLayout SSBOSetLayout;
};
