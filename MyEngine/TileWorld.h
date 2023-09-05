#pragma once

#include <vector>
#include <stdint.h>
#include <cassert>
#include <memory>
#include <glm/glm.hpp>
#include <algorithm>

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

constexpr float ambiantLight = 0.02f;

const static int maxChunkUpdatesPerFrame = 10;
const static int maxLightsPerChunk = 100;

struct chunkLightingUpdateinfo {
	uint32_t chunkIndex;
	int lightCount;
	alignas(16) glm::vec4 lightPositions[maxLightsPerChunk]; // aligned to 16 bytes for vec2
};
static_assert(sizeof(chunkLightingUpdateinfo) % 16 == 0);

constexpr int coolsize = sizeof(chunkLightingUpdateinfo);

class TileWorld {

public:


	// by chunk and then global position
	std::vector< std::vector<glm::vec2>> torchPositions;

	//std::vector<glm::vec2> torchPositions;

	std::vector<blockID> mapData;

	// for gpu updating
	uint32_t minDirtyIndex = UINT32_MAX;
	uint32_t maxDirtyIndex = 0;
	uint32_t minDirtyLightingIndex = UINT32_MAX;
	uint32_t maxDirtyLightingIndex = 0;
	std::vector<bool> chunkDirtyFlags;
	std::vector<bool> chunkLightingDirtyFlags;

	TileWorld(std::shared_ptr<VKEngine> engine) : engine(engine) {
		mapData = std::vector<blockID>(mapCount, 0);
		chunkDirtyFlags = std::vector<bool>(chunkCount, false);
		chunkLightingDirtyFlags = std::vector<bool>(chunkCount, false);
		torchPositions.resize(chunkCount);
	};

	void saveToDisk(std::string filepath) {
		FILE* f = fopen(filepath.c_str(), "wb");
		fwrite(mapData.data(), sizeof(blockID), mapCount, f);
		fclose(f);
	};
	void loadFromDisk(std::string filepath) {
		FILE* f = fopen(filepath.c_str(), "rb");
		fread(mapData.data(), sizeof(blockID), mapCount, f);
		fclose(f);
	};

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
		return mapData[chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize)] & 0xFFFF;
	};

	void setTile(uint32_t x, uint32_t y, blockID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chunkIndexOffset = chunk * chunkTileCount;
		uint32_t index = chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		mapData[index] = (mapData[index] & (0xFFFF << 16)) | block;
		chunkDirtyFlags[chunk] = true;
		minDirtyIndex = chunk < minDirtyIndex ? chunk : minDirtyIndex;
		maxDirtyIndex = chunk > maxDirtyIndex ? chunk : maxDirtyIndex;
	};

	// same as setTile but does not mark the chunk as dirty
	void preloadTile(uint32_t x, uint32_t y, blockID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chuckIndexOffset = chunk * chunkTileCount;
		mapData[chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize)] = block;
	};

	uint8_t getAdjacencyHash(uint32_t x, uint32_t y) {
		uint8_t hash = 0;
		hash |= (uint8_t)(getTile(x, y - 1) != 1023) << 0;
		hash |= (uint8_t)(getTile(x - 1, y) != 1023) << 1;
		hash |= (uint8_t)(getTile(x + 1, y) != 1023) << 2;
		hash |= (uint8_t)(getTile(x, y + 1) != 1023) << 3;
		return hash;
	};

	void preloadBrightness(uint32_t x, uint32_t y, uint16_t brightness) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chunkIndexOffset = chunk * chunkTileCount;
		uint32_t index = chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		mapData[index] = (mapData[index] & 0xFFFF) | (brightness << 16);
	};

	void setBrightness(uint32_t x, uint32_t y, uint16_t brightness) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chunkIndexOffset = chunk * chunkTileCount;
		uint32_t index = chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		mapData[index] = (mapData[index] & 0xFFFF) | (brightness << 16);
		chunkLightingDirtyFlags[chunk] = true;
		minDirtyLightingIndex = chunk < minDirtyLightingIndex ? chunk : minDirtyLightingIndex;
		maxDirtyLightingIndex = chunk > maxDirtyLightingIndex ? chunk : maxDirtyLightingIndex;

		chunkDirtyFlags[chunk] = true;
		minDirtyIndex = chunk < minDirtyIndex ? chunk : minDirtyIndex;
		maxDirtyIndex = chunk > maxDirtyIndex ? chunk : maxDirtyIndex;
	};

	// flag surrounding chunks as dirty 
	void setTorch(uint32_t x, uint32_t y) {

		uint32_t _cx = (x / chunkSize);
		uint32_t _cy = (y / chunkSize);
		uint32_t chunk = +_cy * chunksX + _cx;
		torchPositions[chunk].push_back({ x, y });

		int _i = x % chunkSize > (chunkSize / 2) ? 0 : -1;
		int _j = y % chunkSize > (chunkSize / 2) ? 0 : -1;

		for (int i = _i; i < _i + 2; i++)
		{
			for (int j = _j; j < _j + 2; j++) {
				uint32_t cx = _cx + i;
				uint32_t cy = _cy + j;
				chunk = cy * chunksX + cx;
				chunkLightingDirtyFlags[chunk] = true;
				minDirtyLightingIndex = chunk < minDirtyLightingIndex ? chunk : minDirtyLightingIndex;
				maxDirtyLightingIndex = chunk > maxDirtyLightingIndex ? chunk : maxDirtyLightingIndex;
				/*chunkDirtyFlags[chunk] = true;
				minDirtyIndex = chunk < minDirtyIndex ? chunk : minDirtyIndex;
				maxDirtyIndex = chunk > maxDirtyIndex ? chunk : maxDirtyIndex;*/
			}

		}
	};

	std::vector<chunkLightingUpdateinfo> chunkLightingJobs;

	void updateLighing() {
		// nothing updated
		if (minDirtyLightingIndex == UINT32_MAX)
			return;

		// could devide each chunk update into quadrents which only check the torches in four surrounding chunks instead of all 9 surrounding chunks
		for (size_t chunk = minDirtyLightingIndex; chunk <= maxDirtyLightingIndex; chunk++) {
			if (chunkLightingDirtyFlags[chunk] == true) {
				chunkLightingDirtyFlags[chunk] = false;

				uint32_t baseIndex = chunk * chunkTileCount;

				//std::vector<glm::vec2>* searchTorches[9];
				uint32_t chunkX = chunk % chunksX;
				uint32_t chunkY = chunk / chunksX;

				chunkLightingUpdateinfo update;
				update.chunkIndex = chunk;

				int t = 0;
				for (int i = -1; i < 2; i++) {
					for (int j = -1; j < 2; j++) {
						uint32_t cx = chunkX + i;
						uint32_t cy = chunkY + j;
						uint32_t chunkSearch = cy * chunksX + cx;
						for (auto& v : torchPositions[chunkSearch])
						{
							if (t >= maxLightsPerChunk)
								continue;
							update.lightPositions[t++] = glm::vec4(v.x, v.y, 0, 0);
						}
						/*std::copy(torchPositions[chunkSearch].begin(), torchPositions[chunkSearch].end(), update.lightPositions + t);
						t += torchPositions[chunkSearch].size();*/
					}
				}
				update.lightCount = t;
				chunkLightingJobs.push_back(update);
			}
		}
	};

	std::vector<chunkLightingUpdateinfo> getLightingUpdateData() {
		return chunkLightingJobs;
	}

	void stageChunkUpdates(VkCommandBuffer commandBuffer) {

		// only able to stage one chunk per frame. Expand to multiple chunks by increasing size of transfer buffer

		// nothing updated
		if (minDirtyIndex == UINT32_MAX)
			return;

		//for (size_t i = 0; i < chunkCount; i++) {
		for (size_t i = minDirtyIndex; i <= maxDirtyIndex; i++) {
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

				return;
			}
		}

		// nothing left to upload, safe to reset dirty indexes
		minDirtyIndex = INT32_MAX;
		maxDirtyIndex = 0;
	};

	VkBuffer _worldMapDeviceBuffer;

	struct ssboObjectData {
		blockID index;
	};

private:



	void createWorldBuffer() {
		engine->createBuffer(sizeof(ssboObjectData) * (TileWorld_MAX_TILES), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, _worldMapDeviceBuffer, worldMapDeviceBufferAllocation, true);
	};
	void createChunkTransferBuffers() {
		// create large buffer for initial upload
		engine->createBuffer(sizeof(ssboObjectData) * (largeChunkCount), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, largeChunkBuffer, largeChunkAllocation);
		vmaMapMemory(engine->allocator, largeChunkAllocation, &largeChunkBufferMapped);

		// create small buffers for individual chunk updates
		engine->createMappedBuffer(sizeof(ssboObjectData) * chunkCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, chunkTransferBuffers);
	};

	std::shared_ptr<VKEngine> engine;

	VkBuffer largeChunkBuffer;
	VmaAllocation largeChunkAllocation;
	void* largeChunkBufferMapped;

	MappedDoubleBuffer chunkTransferBuffers;

	VmaAllocation worldMapDeviceBufferAllocation;

};