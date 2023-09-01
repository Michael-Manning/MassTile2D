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

class TilemapPL :public  Pipeline {
public:

	struct ssboObjectData {
		blockID index;
	};

	TilemapPL(std::shared_ptr<VKEngine>& engine) : Pipeline(engine) {
		mapData.resize(TilemapPL_MAX_TILES);
		chunkDirtyFlags = std::vector<bool>(chunkCount, false);
	}


	void CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) override;
	void createDescriptorSetLayout() override;
	void createDescriptorSets(MappedDoubleBuffer& cameradb);
	void createVertices() override;
	void createWorldBuffer();
	void createChunkTransferBuffers();

	void recordCommandBuffer(VkCommandBuffer commandBuffer);

	inline blockID getTile(uint32_t x, uint32_t y) {
		int cx = (x / chunkSize);
		int cy = (y / chunkSize);
		int chunk = cy * chunksX + cx;
		int chuckIndexOffset = chunk * chunkTileCount;
		return mapData[chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize)];
	}

	void setTile(uint32_t x, uint32_t y, blockID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chunkIndexOffset = chunk * chunkTileCount;
		mapData[chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize)] = block;
		chunkDirtyFlags[chunk] = true;
		minDirtyIndex = chunk < minDirtyIndex ? chunk : minDirtyIndex;
		maxDirtyIndex = chunk > maxDirtyIndex ? chunk : maxDirtyIndex;
	};

	void preloadTile(uint32_t x, uint32_t y, blockID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chuckIndexOffset = chunk * chunkTileCount;
		mapData[chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize)] = block;
	};

	int minDirtyIndex = INT32_MAX;
	int maxDirtyIndex = -1;
	std::vector<bool> chunkDirtyFlags;
	std::vector<blockID> mapData;

	std::optional<Texture> textureAtlas;

	void copyToLargeChunkTransferbuffer(uint32_t* data) {
		memcpy(largeChunkBufferMapped, data, sizeof(ssboObjectData) * largeChunkCount);
	}

	void copyLargeChunkToDevice(int chunkIndex) {
		engine->copyBuffer(largeChunkBuffer, worldMapDeviceBuffer, sizeof(ssboObjectData) * largeChunkCount, chunkIndex * sizeof(ssboObjectData) * largeChunkCount);
	}

	void stageChunkUpdates(VkCommandBuffer commandBuffer);

private:
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout SSBOSetLayout;


	VkBuffer largeChunkBuffer;
	VmaAllocation largeChunkAllocation;
	void* largeChunkBufferMapped;

	MappedDoubleBuffer chunkTransferBuffers;

	VkBuffer worldMapDeviceBuffer;
	VmaAllocation worldMapDeviceBufferAllocation;
};
