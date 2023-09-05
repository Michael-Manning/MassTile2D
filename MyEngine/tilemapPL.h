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

	TilemapPL(std::shared_ptr<VKEngine>& engine, VertexMeshBuffer quadMesh, std::shared_ptr<TileWorld> world) : Pipeline(engine), quadMesh(quadMesh), world(world) {
	}

	void CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc, MappedDoubleBuffer& cameradb);
	void recordCommandBuffer(VkCommandBuffer commandBuffer);


	void setTextureAtlas(Texture textureAtlas) {

		// fill in missing data of descriptor set builder before submitting

		this->textureAtlas = textureAtlas;
		builderDescriptorSetsDetails[1].texture = &this->textureAtlas.value();

		std::array<VkBuffer, FRAMES_IN_FLIGHT> worldMapDeviceBuferRef = { world->_worldMapDeviceBuffer, world->_worldMapDeviceBuffer };
		builderDescriptorSetsDetails[2].doubleBuffer = &worldMapDeviceBuferRef;

		buildDescriptorSets();

	}

	std::optional<Texture> textureAtlas;
private:


	VKUtil::UBOUploader<cameraUBO_s> cameraUploader;

	std::shared_ptr<TileWorld> world = nullptr;

	VertexMeshBuffer quadMesh;

	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout SSBOSetLayout;
};
