#pragma once

#include <vector>
#include <stdint.h>
#include <cassert>
#include <memory>
#include <algorithm>
#include <iostream>

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

constexpr float ambiantLight = 1.00f;

const static int maxChunkTransfersPerFrame = 16;
const static int maxChunkBaseLightingUpdatesPerFrame = 16;
const static int maxLightsPerChunk = 100;

struct chunkLightingUpdateinfo {
	uint32_t chunkIndex;
	int lightCount;
	alignas(16) glm::vec4 lightPositions[maxLightsPerChunk]; // std140 alignment
};
static_assert(sizeof(chunkLightingUpdateinfo) % 16 == 0);

constexpr int coolsize = sizeof(chunkLightingUpdateinfo);

class TileWorld {

private:

	struct chunkDirtyState {
		uint32_t minIndex = UINT32_MAX;
		uint32_t maxIndex = 0;
		std::vector<bool> dirtyFlags; // try replacing with set
	};

	inline void invalidateWholeState(chunkDirtyState& state) {
		std::fill(state.dirtyFlags.begin(), state.dirtyFlags.end(), true);
		state.minIndex = 0;
		state.maxIndex = chunkCount - 1;
	}

public:


	// by chunk and then global position
	std::vector< std::vector<glm::vec2>> torchPositions;

	//std::vector<glm::vec2> torchPositions;

	std::vector<blockID> mapData;
	std::vector<blockID> bgMapData;

	// for gpu updating
	chunkDirtyState tileDirtyState;
	chunkDirtyState baseLightingDirtyState;
	chunkDirtyState blurLightingDirtyState;

	TileWorld(VKEngine* engine) : engine(engine) {
		mapData = std::vector<blockID>(mapCount, 0 | ((int)(ambiantLight * 255) << 16));
		bgMapData = std::vector<blockID>(mapCount, 0);
		tileDirtyState.dirtyFlags = std::vector<bool>(chunkCount, false);
		baseLightingDirtyState.dirtyFlags = std::vector<bool>(chunkCount, false);
		blurLightingDirtyState.dirtyFlags = std::vector<bool>(chunkCount, false);
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
		memcpy(largeChunkBufferMapped, data, sizeof(worldTile_ssbo) * largeChunkCount);
	};

	void copyLargeFGChunkToDevice(int chunkIndex) {
		engine->copyBuffer(largeChunkBuffer, _worldMapFGDeviceBuffer, sizeof(worldTile_ssbo) * largeChunkCount, mapOffset * sizeof(worldTile_ssbo) + chunkIndex * sizeof(worldTile_ssbo) * largeChunkCount);
	};
	void copyLargeBGChunkToDevice(int chunkIndex) {
		engine->copyBuffer(largeChunkBuffer, _worldMapBGDeviceBuffer, sizeof(worldTile_ssbo) * largeChunkCount, mapOffset * sizeof(worldTile_ssbo) + chunkIndex * sizeof(worldTile_ssbo) * largeChunkCount);
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
		invalidateWholeState(baseLightingDirtyState);
		invalidateWholeState(blurLightingDirtyState);
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
		return cy * chunksX + cx;
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

	inline void InvalidateLighting(int chunk) {
		baseLightingDirtyState.dirtyFlags[chunk] = true;
		baseLightingDirtyState.minIndex = chunk < baseLightingDirtyState.minIndex ? chunk : baseLightingDirtyState.minIndex;
		baseLightingDirtyState.maxIndex = chunk > baseLightingDirtyState.maxIndex ? chunk : baseLightingDirtyState.maxIndex;
		blurLightingDirtyState.dirtyFlags[chunk] = true;
		blurLightingDirtyState.minIndex = chunk < blurLightingDirtyState.minIndex ? chunk : blurLightingDirtyState.minIndex;
		blurLightingDirtyState.maxIndex = chunk > blurLightingDirtyState.maxIndex ? chunk : blurLightingDirtyState.maxIndex;
	}

	inline void FlagChunkForUpload(int chunk) {
		tileDirtyState.dirtyFlags[chunk] = true;
		tileDirtyState.minIndex = chunk < tileDirtyState.minIndex ? chunk : tileDirtyState.minIndex;
		tileDirtyState.maxIndex = chunk > tileDirtyState.maxIndex ? chunk : tileDirtyState.maxIndex;
	}

	void UpdateChunk(int chunk) {
		assert(chunk >= 0 && chunk < chunkCount);
		FlagChunkForUpload(chunk);
		InvalidateLighting(chunk);
	}

	inline void setTile(glm::ivec2 tile, tileID block) { setTile(tile.x, tile.y, block); }
	void setTile(uint32_t x, uint32_t y, tileID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chunkIndexOffset = chunk * chunkTileCount;
		uint32_t index = mapOffset + chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize);
		index %= mapCount;

		//mapData[index] = (mapData[index] & (0xFFFF << 16)) | block;
		mapData[index] = block;
		FlagChunkForUpload(chunk);

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
				InvalidateLighting(chunk);
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

		// this function is probably flawed now. Brightness won't be persistent if recalculated by compute

		FlagChunkForUpload(chunk);
		InvalidateLighting(chunk);
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
				InvalidateLighting(chunk);
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
				InvalidateLighting(chunk);
			}

		}
	};

	std::vector<chunkLightingUpdateinfo> baseChunkLightingJobs;
	std::vector<chunkLightingUpdateinfo> blurChunkLightingJobs;

	void updateBaseLighting() {
		ZoneScoped;

		// nothing updated
		if (baseLightingDirtyState.minIndex == UINT32_MAX)
			return;

		if (baseChunkLightingJobs.size() >= maxChunkBaseLightingUpdatesPerFrame)
			return;

		// could devide each chunk update into quadrents which only check the torches in four surrounding chunks instead of all 9 surrounding chunks
		for (size_t chunk = baseLightingDirtyState.minIndex; chunk <= baseLightingDirtyState.maxIndex; chunk++) {
			if (baseLightingDirtyState.dirtyFlags[chunk] == true) {
				baseLightingDirtyState.dirtyFlags[chunk] = false;

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
				//std::cout << "light update chunk " << chunk << "\n";
				baseChunkLightingJobs.push_back(update);

				if (baseChunkLightingJobs.size() >= maxChunkBaseLightingUpdatesPerFrame)
					return;
			}

			baseLightingDirtyState.minIndex = chunk;
		}

		// nothing left to upload, safe to reset dirty indexes
		baseLightingDirtyState.minIndex = UINT32_MAX;
		baseLightingDirtyState.maxIndex = 0;
	}

	void updateLighing() {
		updateBaseLighting();
		//updateBlurLighting();
	};

	std::vector<chunkLightingUpdateinfo> getLightingUpdateData() {
		ZoneScoped;
		return baseChunkLightingJobs;
	}

	void stageChunkUpdates(vk::CommandBuffer commandBuffer) {

		// nothing updated
		if (tileDirtyState.minIndex == UINT32_MAX)
			return;

		std::vector<vk::BufferCopy> copyRegions;
		copyRegions.reserve(maxChunkTransfersPerFrame);

		for (size_t chunk = tileDirtyState.minIndex; chunk <= tileDirtyState.maxIndex; chunk++) {
			if (tileDirtyState.dirtyFlags[chunk] == true) {
				tileDirtyState.dirtyFlags[chunk] = false;

				//std::cout << "transfering chunk " << i << "\n";

				memcpy(chunkTransferBuffers.buffersMapped[engine->currentFrame] + copyRegions.size() * chunkTileCount,
					mapData.data() + chunk * chunkTileCount, sizeof(worldTile_ssbo) * chunkTileCount);

				size_t dstOffset = chunk * chunkTileCount;

				vk::BufferCopy& copyRegion = copyRegions.emplace_back();
				copyRegion.size = chunkTileCount * sizeof(worldTile_ssbo);
				copyRegion.dstOffset = dstOffset * sizeof(worldTile_ssbo);
				copyRegion.srcOffset = (copyRegions.size() - 1) * chunkTileCount * sizeof(worldTile_ssbo);


				if (copyRegions.size() == maxChunkTransfersPerFrame) {
					break;
				}
			}

			tileDirtyState.minIndex = chunk;
		}

		assert(copyRegions.size() > 0);

		commandBuffer.copyBuffer(chunkTransferBuffers.buffers[engine->currentFrame], _worldMapFGDeviceBuffer, copyRegions.size(), copyRegions.data());


		//must insure transfer is complete before lighting compute shader can access the world buffer
		{
			vk::BufferMemoryBarrier barrier;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.buffer = _worldMapFGDeviceBuffer;
			barrier.offset = 0;
			barrier.size = VK_WHOLE_SIZE; // Could be changed to a portion, but I don't know if there would be a benefit

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eComputeShader,
				static_cast<vk::DependencyFlags>(0),
				0, nullptr,
				1, &barrier,
				0, nullptr
			);
		}



		// nothing left to upload, safe to reset dirty indexes
		if (tileDirtyState.minIndex == tileDirtyState.maxIndex) {
			tileDirtyState.minIndex = UINT32_MAX;
			tileDirtyState.maxIndex = 0;
		}
	};

	vk::Buffer _worldMapFGDeviceBuffer;
	vk::Buffer _worldMapBGDeviceBuffer;

	struct worldTile_ssbo {
		tileID index;
	};

private:



	void createWorldBuffer() {
		// create foreground and background VRAM buffers
		engine->createBuffer(sizeof(worldTile_ssbo) * (mapCount), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, _worldMapFGDeviceBuffer, worldMapFGDeviceBufferAllocation, true);
		engine->createBuffer(sizeof(worldTile_ssbo) * (mapCount), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, _worldMapBGDeviceBuffer, worldMapBGDeviceBufferAllocation, true);
	};
	void createChunkTransferBuffers() {

		// create large buffer for initial upload
		engine->createBuffer(sizeof(worldTile_ssbo) * (largeChunkCount), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, largeChunkBuffer, largeChunkAllocation);
		vmaMapMemory(engine->allocator, largeChunkAllocation, &largeChunkBufferMapped);

		// create small buffers for individual chunk updates
		engine->createMappedBuffer(sizeof(worldTile_ssbo) * chunkTileCount * maxChunkTransfersPerFrame, vk::BufferUsageFlagBits::eTransferSrc, chunkTransferBuffers);
	};

	VKEngine* engine;

	vk::Buffer largeChunkBuffer;
	VmaAllocation largeChunkAllocation;
	void* largeChunkBufferMapped;

	MappedDoubleBuffer<worldTile_ssbo> chunkTransferBuffers;

	VmaAllocation worldMapFGDeviceBufferAllocation;
	VmaAllocation worldMapBGDeviceBufferAllocation;

};