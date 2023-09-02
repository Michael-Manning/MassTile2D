#pragma once

#include <vector>
#include <stdint.h>
#include <cassert>
#include <memory>

#include "typedefs.h"
#include "VKEngine.h"

constexpr int TileWorld_MAX_TILES = 1048576 * 2;
constexpr int largeChunkCount = 131072;
constexpr int mapW = 1024 * 2;
constexpr int mapH = 1024 * 1;
constexpr int mapCount = mapW * mapH;

constexpr int chunkSize = 32;
constexpr int chunkTileCount = chunkSize * chunkSize;


constexpr int chunksX = mapW / chunkSize;
constexpr int chunksY = mapH / chunkSize;
constexpr int chunkCount = chunksX * chunksY;

// size of an individual size in wold units
constexpr float tileWorldSize = 0.25f;

static_assert(mapCount == TileWorld_MAX_TILES);
static_assert(mapW% chunkSize == 0);
static_assert(mapH% chunkSize == 0);

class TileWorld {

public:
	std::vector<blockID> mapData;

	// for gpu updating
	int minDirtyIndex = INT32_MAX;
	int maxDirtyIndex = -1;
	std::vector<bool> chunkDirtyFlags;

	TileWorld(std::shared_ptr<VKEngine> engine) : engine(engine) {
		mapData = std::vector<blockID>(mapCount, 0);
		chunkDirtyFlags = std::vector<bool>(chunkCount, false);
	}

	void copyToLargeChunkTransferbuffer(uint32_t* data) {
		memcpy(largeChunkBufferMapped, data, sizeof(ssboObjectData) * largeChunkCount);
	};

	void copyLargeChunkToDevice(int chunkIndex) {
		engine->copyBuffer(largeChunkBuffer, _worldMapDeviceBuffer, sizeof(ssboObjectData) * largeChunkCount, chunkIndex * sizeof(ssboObjectData) * largeChunkCount);
	};

	void AllocateVulkanResources() {
		createWorldBuffer();
		createChunkTransferBuffers();

	};

	void uploadWorldPreloadData() {
		constexpr int transferCount = TileWorld_MAX_TILES / largeChunkCount;
		for (size_t i = 0; i < transferCount; i++) {

			copyToLargeChunkTransferbuffer(mapData.data() + i * largeChunkCount);
			copyLargeChunkToDevice(i);
		}
	};


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

	void stageChunkUpdates(VkCommandBuffer commandBuffer) {

		// ignoring chunk update range optimization for first test

		for (size_t i = 0; i < chunkCount; i++) {
			if (chunkDirtyFlags[i] == true) {
				memcpy(chunkTransferBuffers.buffersMapped[engine->currentFrame], mapData.data() + i * chunkTileCount, sizeof(ssboObjectData) * chunkTileCount);
				chunkDirtyFlags[i] = false;

				{
					VkBufferCopy copyRegion{};
					copyRegion.size = chunkTileCount * sizeof(ssboObjectData);
					copyRegion.dstOffset = i * chunkTileCount * sizeof(ssboObjectData);
					copyRegion.srcOffset = 0;

					// expand by increasing size of transfer buffer. Upload multiple chunks by specifying multiple copy regions
					vkCmdCopyBuffer(commandBuffer, chunkTransferBuffers.buffers[engine->currentFrame], _worldMapDeviceBuffer, 1, &copyRegion);
				}
			}
		}
	}

	VkBuffer _worldMapDeviceBuffer;

	struct ssboObjectData {
		blockID index;
	};

private:



	void createWorldBuffer() {
		engine->createBuffer(sizeof(ssboObjectData) * (TileWorld_MAX_TILES), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, _worldMapDeviceBuffer, worldMapDeviceBufferAllocation, true);
	}
	void createChunkTransferBuffers() {
		// create large buffer for initial upload
		engine->createBuffer(sizeof(ssboObjectData) * (largeChunkCount), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, largeChunkBuffer, largeChunkAllocation);
		vmaMapMemory(engine->allocator, largeChunkAllocation, &largeChunkBufferMapped);

		// create small buffers for individual chunk updates
		engine->createMappedBuffer(sizeof(ssboObjectData) * chunkCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, chunkTransferBuffers);
	}

	std::shared_ptr<VKEngine> engine;

	VkBuffer largeChunkBuffer;
	VmaAllocation largeChunkAllocation;
	void* largeChunkBufferMapped;

	MappedDoubleBuffer chunkTransferBuffers;

	VmaAllocation worldMapDeviceBufferAllocation;

};