#pragma once

#include <vector>
#include <stdint.h>
#include <cassert>
#include <memory>
#include <algorithm>

#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>

#include "typedefs.h"
#include "VKEngine.h"

constexpr int largeChunkCount = 131072;
constexpr int mapW = 1024 * 2;
constexpr int mapH = 1024 * 1;
//constexpr int mapPadding = (mapW + 2) * 2 + (mapH + 2) * 2 * 2;
constexpr int mapPadding = 0;
constexpr int mapOffset = mapPadding / 2;
constexpr int mapCount = mapW * mapH + +mapPadding;

constexpr int chunkSize = 32;
constexpr int chunkTileCount = chunkSize * chunkSize;


constexpr int chunksX = mapW / chunkSize;
constexpr int chunksY = mapH / chunkSize;
constexpr int chunkCount = chunksX * chunksY;

// size of an individual size in wold units
constexpr float tileWorldSize = 0.25f;

static_assert(mapW% chunkSize == 0);
static_assert(mapH% chunkSize == 0);

constexpr float ambiantLight = 0.05f;

const static int maxChunkUpdatesPerFrame = 16;
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
	std::vector<blockID> bgMapData;

	// for gpu updating
	uint32_t minDirtyIndex = UINT32_MAX;
	uint32_t maxDirtyIndex = 0;
	uint32_t minDirtyLightingIndex = UINT32_MAX;
	uint32_t maxDirtyLightingIndex = 0;
	std::vector<bool> chunkDirtyFlags;
	std::vector<bool> chunkLightingDirtyFlags;

	TileWorld(VKEngine* engine) : engine(engine) {
		mapData = std::vector<blockID>(mapCount, 0 | ((int)(ambiantLight * 255) << 16));
		bgMapData = std::vector<blockID>(mapCount, 0);
		chunkDirtyFlags = std::vector<bool>(chunkCount, false);
		chunkLightingDirtyFlags = std::vector<bool>(chunkCount, false);
		torchPositions.resize(chunkCount);
	};

	void saveToDisk(std::string filepath) {
		FILE* f = fopen(filepath.c_str(), "wb");
		fwrite(mapData.data(), sizeof(blockID), mapCount, f);
		fwrite(bgMapData.data(), sizeof(blockID), mapCount, f);
		fclose(f);
	};
	void loadFromDisk(std::string filepath) {
		FILE* f = fopen(filepath.c_str(), "rb");
		fread(mapData.data(), sizeof(blockID), mapCount, f);
		fread(bgMapData.data(), sizeof(blockID), mapCount, f);
		fclose(f);
	};

	void copyToLargeChunkTransferbuffer(uint32_t* data) {
		memcpy(largeChunkBufferMapped, data, sizeof(ssboObjectData) * largeChunkCount);
	};

	void copyLargeFGChunkToDevice(int chunkIndex) {
		engine->copyBuffer(largeChunkBuffer, _worldMapFGDeviceBuffer, sizeof(ssboObjectData) * largeChunkCount, mapOffset * sizeof(ssboObjectData) + chunkIndex * sizeof(ssboObjectData) * largeChunkCount);
	};
	void copyLargeBGChunkToDevice(int chunkIndex) {
		engine->copyBuffer(largeChunkBuffer, _worldMapBGDeviceBuffer, sizeof(ssboObjectData) * largeChunkCount, mapOffset * sizeof(ssboObjectData) + chunkIndex * sizeof(ssboObjectData) * largeChunkCount);
	};

	void AllocateVulkanResources() {
		createWorldBuffer();
		createChunkTransferBuffers();

	};

	void uploadWorldPreloadData() {

		constexpr int transferCount = (mapCount - mapPadding) / largeChunkCount;
		for (size_t i = 0; i < transferCount; i++) {
			copyToLargeChunkTransferbuffer(&mapData[(mapOffset + i * largeChunkCount)]);
			copyLargeFGChunkToDevice(i);
		}
		for (size_t i = 0; i < transferCount; i++) {
			copyToLargeChunkTransferbuffer(&bgMapData[(mapOffset + i * largeChunkCount)]);
			copyLargeBGChunkToDevice(i);
		}
	};

	void FullLightingUpdate() {
		std::fill(chunkLightingDirtyFlags.begin(), chunkLightingDirtyFlags.end(), true);
		minDirtyLightingIndex = 0;
		maxDirtyLightingIndex = chunkCount - 1;
	};

	inline tileID getTile(uint32_t x, uint32_t y) const {
		int cx = (x / chunkSize);
		int cy = (y / chunkSize);
		int chunk = cy * chunksX + cx;
		int chuckIndexOffset = chunk * chunkTileCount;
		uint32_t index = mapOffset + chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		index %= mapCount;
		return mapData[index] & 0xFFFF;
	};
	inline tileID getTile(glm::ivec2 tile) const { return getTile(tile.x, tile.y); }

	inline uint32_t GetChunk(const glm::ivec2 tile) const {
		uint32_t cx = (tile.x / chunkSize);
		uint32_t cy = (tile.y / chunkSize);
		return cy* chunksX + cx;
	}

	inline glm::ivec2 WorldPosTile(const glm::vec2 pos) const {
		return 	{
			pos.x / tileWorldSize + mapW / 2,
			pos.y / tileWorldSize + mapH / 2
		};
	}

	inline glm::vec2 TileWorldPos(const glm::ivec2 tile) const {
		return{
			(tile.x - mapW / 2) * tileWorldSize,
			(tile.y - mapH / 2) * tileWorldSize
		};
	}

	void UpdateChunk(int chunk) {
		assert(chunk >= 0 && chunk < chunkCount);
		chunkDirtyFlags[chunk] = true;
		minDirtyIndex = chunk < minDirtyIndex ? chunk : minDirtyIndex;
		maxDirtyIndex = chunk > maxDirtyIndex ? chunk : maxDirtyIndex;
		chunkLightingDirtyFlags[chunk] = true;
		minDirtyLightingIndex = chunk < minDirtyLightingIndex ? chunk : minDirtyLightingIndex;
		maxDirtyLightingIndex = chunk > maxDirtyLightingIndex ? chunk : maxDirtyLightingIndex;
	}

	inline void setTile(glm::ivec2 tile, tileID block) { setTile(tile.x, tile.y, block); }
	void setTile(uint32_t x, uint32_t y, tileID block) {
		//y = mapH - y - 1;
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chunkIndexOffset = chunk * chunkTileCount;
		uint32_t index = mapOffset + chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		index %= mapCount;
		mapData[index] = (mapData[index] & (0xFFFF << 16)) | block;
		chunkDirtyFlags[chunk] = true;
		minDirtyIndex = chunk < minDirtyIndex ? chunk : minDirtyIndex;
		maxDirtyIndex = chunk > maxDirtyIndex ? chunk : maxDirtyIndex;


		// NOTE: Should only need to update 4 chunks depending on the quadrant within the chunk
		// which the tile was set. Same for moving torches if the quadrant it moved from is the same as it's destination
		// update lighting
		for (int i = -1; i < 2; i++)
		{
			for (int j = -1; j < 2; j++) {
				uint32_t _cx = cx + i;
				uint32_t _cy = cy + j;
				chunk = _cy * chunksX + _cx;
				chunk %= chunkCount;
				chunkLightingDirtyFlags[chunk] = true;
				minDirtyLightingIndex = chunk < minDirtyLightingIndex ? chunk : minDirtyLightingIndex;
				maxDirtyLightingIndex = chunk > maxDirtyLightingIndex ? chunk : maxDirtyLightingIndex;
			}
		}

	};

	// same as setTile but does not mark the chunk as dirty
	void preloadTile(uint32_t x, uint32_t y, tileID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chuckIndexOffset = chunk * chunkTileCount;
		uint32_t index = mapOffset + chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		index %= mapCount;
		mapData[index] = block;
	};
	void preloadBGTile(uint32_t x, uint32_t y, tileID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chuckIndexOffset = chunk * chunkTileCount;
		uint32_t index = mapOffset + chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		index %= mapCount;
		bgMapData[index] = block;
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
		uint32_t index = mapOffset + chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		index %= mapCount;
		mapData[index] = (mapData[index] & 0xFFFF) | (brightness << 16);
	};

	void setBrightness(uint32_t x, uint32_t y, uint16_t brightness) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chunkIndexOffset = chunk * chunkTileCount;
		uint32_t index = mapOffset + chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		index %= mapCount;
		mapData[index] = (mapData[index] & 0xFFFF) | (brightness << 16);
		chunkLightingDirtyFlags[chunk] = true;

		minDirtyLightingIndex = chunk < minDirtyLightingIndex ? chunk : minDirtyLightingIndex;
		maxDirtyLightingIndex = chunk > maxDirtyLightingIndex ? chunk : maxDirtyLightingIndex;

		chunkDirtyFlags[chunk] = true;
		minDirtyIndex = chunk < minDirtyIndex ? chunk : minDirtyIndex;
		maxDirtyIndex = chunk > maxDirtyIndex ? chunk : maxDirtyIndex;
	};


	glm::vec2 movingTorch = glm::vec2(0, 0);
	bool useMovingTorch = false;

	void setMovingTorch(glm::vec2 pos, bool enabled) {
		movingTorch = pos;
		useMovingTorch = enabled;


		uint32_t _cx = (pos.x / chunkSize);
		uint32_t _cy = (pos.y / chunkSize);
		uint32_t chunk = +_cy * chunksX + _cx;

		for (int i = -1; i < 2; i++)
		{
			for (int j = -1; j < 2; j++) {
				uint32_t cx = _cx + i;
				uint32_t cy = _cy + j;
				chunk = cy * chunksX + cx;
				chunk %= chunkCount;
				chunkLightingDirtyFlags[chunk] = true;
				minDirtyLightingIndex = chunk < minDirtyLightingIndex ? chunk : minDirtyLightingIndex;
				maxDirtyLightingIndex = chunk > maxDirtyLightingIndex ? chunk : maxDirtyLightingIndex;
			}
		}

	};

	// flag surrounding chunks as dirty 
	void setTorch(uint32_t x, uint32_t y) {

		uint32_t _cx = (x / chunkSize);
		uint32_t _cy = (y / chunkSize);
		uint32_t chunk = +_cy * chunksX + _cx;
		torchPositions[chunk].push_back({ x, y });

		/*int _i = x % chunkSize > (chunkSize / 2) ? 0 : -1;
		int _j = y % chunkSize > (chunkSize / 2) ? 0 : -1;

		for (int i = _i; i < _i + 2; i++)
		{
			for (int j = _j; j < _j + 2; j++) {*/

		for (int i = -1; i < 2; i++)
		{
			for (int j = -1; j < 2; j++) {
				uint32_t cx = _cx + i;
				uint32_t cy = _cy + j;
				chunk = cy * chunksX + cx;
				chunk %= chunkCount;
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
		ZoneScoped;

		// nothing updated
		if (minDirtyLightingIndex == UINT32_MAX)
			return;

		if (chunkLightingJobs.size() >= maxChunkUpdatesPerFrame)
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
						chunkSearch %= chunkCount;
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
				if (useMovingTorch) {
					update.lightPositions[t++] = glm::vec4(movingTorch.x, movingTorch.y, 0.0f, 0.0f);
				}
				update.lightCount = t;
				chunkLightingJobs.push_back(update);

				if (chunkLightingJobs.size() >= maxChunkUpdatesPerFrame)
					return;
			}

			minDirtyLightingIndex = chunk;
		}

		// nothing left to upload, safe to reset dirty indexes
		minDirtyLightingIndex = UINT32_MAX;
		maxDirtyLightingIndex = 0;
	};

	std::vector<chunkLightingUpdateinfo> getLightingUpdateData() {
		ZoneScoped;
		return chunkLightingJobs;
	}

	void stageChunkUpdates(vk::CommandBuffer commandBuffer) {

		// only able to stage one chunk per frame. Expand to multiple chunks by increasing size of transfer buffer

		// nothing updated
		if (minDirtyIndex == UINT32_MAX)
			return;

		//for (size_t i = 0; i < chunkCount; i++) {
		for (size_t i = minDirtyIndex; i <= maxDirtyIndex; i++) {
			if (chunkDirtyFlags[i] == true) {
				memcpy(chunkTransferBuffers.buffersMapped[engine->currentFrame], mapData.data() + mapOffset + i * chunkTileCount, sizeof(ssboObjectData) * chunkTileCount);
				chunkDirtyFlags[i] = false;

				{
					vk::BufferCopy copyRegion{};
					copyRegion.size = chunkTileCount * sizeof(ssboObjectData);
					copyRegion.dstOffset = i * chunkTileCount * sizeof(ssboObjectData);
					copyRegion.srcOffset = 0;

					// expand by increasing size of transfer buffer. Upload multiple chunks by specifying multiple copy regions
					commandBuffer.copyBuffer(chunkTransferBuffers.buffers[engine->currentFrame], _worldMapFGDeviceBuffer, 1, &copyRegion);
				}

				return;
			}
		}

		// nothing left to upload, safe to reset dirty indexes
		minDirtyIndex = UINT32_MAX;
		maxDirtyIndex = 0;
	};

	vk::Buffer _worldMapFGDeviceBuffer;
	vk::Buffer _worldMapBGDeviceBuffer;

	struct ssboObjectData {
		blockID index;
	};

private:



	void createWorldBuffer() {
		// create foreground and background VRAM buffers
		engine->createBuffer(sizeof(ssboObjectData) * (mapCount), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, _worldMapFGDeviceBuffer, worldMapFGDeviceBufferAllocation, true);
		engine->createBuffer(sizeof(ssboObjectData) * (mapCount), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, _worldMapBGDeviceBuffer, worldMapBGDeviceBufferAllocation, true);
	};
	void createChunkTransferBuffers() {
		// create large buffer for initial upload
		engine->createBuffer(sizeof(ssboObjectData) * (largeChunkCount), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, largeChunkBuffer, largeChunkAllocation);
		vmaMapMemory(engine->allocator, largeChunkAllocation, &largeChunkBufferMapped);

		// create small buffers for individual chunk updates
		engine->createMappedBuffer(sizeof(ssboObjectData) * chunkCount, vk::BufferUsageFlagBits::eTransferSrc, chunkTransferBuffers);
	};

	VKEngine* engine;

	vk::Buffer largeChunkBuffer;
	VmaAllocation largeChunkAllocation;
	void* largeChunkBufferMapped;

	MappedDoubleBuffer<void> chunkTransferBuffers;

	VmaAllocation worldMapFGDeviceBufferAllocation;
	VmaAllocation worldMapBGDeviceBufferAllocation;

};