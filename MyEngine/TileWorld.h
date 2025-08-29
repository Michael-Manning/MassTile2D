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
#include "RingBuffer.h"

constexpr int LargeTileWorldWidth = 1024 * 2;
constexpr int LargeTileWorldHeight = 1024;

constexpr int SmallTileWorldWidth = 128;
constexpr int SmallTileWorldHeight = 128;


//constexpr int mapPadding = (mapW + 2) * 2 + (mapH + 2) * 2 * 2;
constexpr int mapPadding = 0;
constexpr int mapOffset = mapPadding / 2;

constexpr int chunkSize = 32;
constexpr int chunkTileCount = chunkSize * chunkSize;

// size of transfer buffer for bulk GPU tile uploads
constexpr int largeChunkCount = 131072; 
static_assert(largeChunkCount % chunkSize == 0);


// size of an individual size in wold units
constexpr float tileWorldBlockSize = 0.25f;

constexpr float ambiantLight = 1.00f;

const static int maxChunkTransfersPerFrame = 16;
const static int maxChunkBaseLightingUpdatesPerFrame = 16;
const static int maxLightsPerChunk = 100;


struct TileWorldDeviceResources {
	DeviceBuffer MapFGBuffer;
	DeviceBuffer MapBGBuffer;
	DeviceBuffer MapLightUpscaleBuffer;
	DeviceBuffer MapLightBlurBuffer;
};


template<int mapW, int mapH>
class TileWorld {

public:
	static constexpr int mapCount = mapW * mapH + mapPadding;
	static constexpr int chunksX = mapW / chunkSize;
	static constexpr int chunksY = mapH / chunkSize;
	static constexpr int chunkCount = chunksX * chunksY;

	static_assert(mapW% chunkSize == 0);
	static_assert(mapH% chunkSize == 0);

private:

	enum class ChunkState {
		Clean,
		Dirty
		//Scheduled // dirty, but scehduled for upload this frame
	};

	struct chunkDirtyState {
		std::vector<ChunkState> flags;
		RingBuffer<int> dirtyIndexes;

		chunkDirtyState() {}
		chunkDirtyState(int capacity) {
			flags = std::vector<ChunkState>(capacity, ChunkState::Clean);
			dirtyIndexes = RingBuffer<int>(capacity);
		}
	};

	inline void invalidateWholeState(chunkDirtyState& state) {
		std::fill(state.flags.begin(), state.flags.end(), ChunkState::Dirty);
		state.dirtyIndexes.iota();
	}

public:

	ShaderTypes::LightingSettings lightingSettings;

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
		tileDirtyState = chunkDirtyState(chunkCount);
		baseLightingDirtyState = chunkDirtyState(chunkCount);
		blurLightingDirtyState = chunkDirtyState(chunkCount);
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
		engine->copyBuffer(largeChunkBuffer, deviceResources.MapFGBuffer.buffer, sizeof(worldTile_ssbo) * largeChunkCount, mapOffset * sizeof(worldTile_ssbo) + chunkIndex * sizeof(worldTile_ssbo) * largeChunkCount);
	};
	void copyLargeBGChunkToDevice(int chunkIndex) {
		engine->copyBuffer(largeChunkBuffer, deviceResources.MapBGBuffer.buffer, sizeof(worldTile_ssbo) * largeChunkCount, mapOffset * sizeof(worldTile_ssbo) + chunkIndex * sizeof(worldTile_ssbo) * largeChunkCount);
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
		int chuckIndexOffset = chunk * chunkTileCount; // could cache when scanning sequentially
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
			pos.x / tileWorldBlockSize + mapW / 2,
			pos.y / tileWorldBlockSize + mapH / 2
		};
	}

	inline glm::vec2 TileWorldPos(const glm::ivec2 tile) const {
		return{
			(tile.x - mapW / 2) * tileWorldBlockSize,
			(tile.y - mapH / 2) * tileWorldBlockSize
		};
	}

	inline void InvalidateLighting(int chunk) {
		if (baseLightingDirtyState.flags[chunk] == ChunkState::Clean) {
			baseLightingDirtyState.flags[chunk] = ChunkState::Dirty;
			baseLightingDirtyState.dirtyIndexes.push(chunk);
		}
		if (blurLightingDirtyState.flags[chunk] == ChunkState::Clean) {
			blurLightingDirtyState.flags[chunk] = ChunkState::Dirty;
			blurLightingDirtyState.dirtyIndexes.push(chunk);
		}
	}

	inline void FlagChunkForUpload(int chunk) {
		if (tileDirtyState.flags[chunk] == ChunkState::Clean) {
			tileDirtyState.flags[chunk] = ChunkState::Dirty;
			tileDirtyState.dirtyIndexes.push(chunk);
		}
	}

	void UpdateChunk(int chunk) {
		assert(chunk >= 0 && chunk < chunkCount);
		//FlagChunkForUpload(chunk);
		//InvalidateLighting(chunk);
		if (blurLightingDirtyState.flags[chunk] == ChunkState::Clean) {
			blurLightingDirtyState.flags[chunk] = ChunkState::Dirty;
			blurLightingDirtyState.dirtyIndexes.push(chunk);
		}
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
		// NOTE: Should only update lighting if the tile is changed from/to a tile that affects lighting. 
		// Shouldn't update lighting going between solids, or from air to entity reserve tile which are both invisible.
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
		hash |= (uint8_t)(getTile(x, y + 1) != 1023) << 0;
		hash |= (uint8_t)(getTile(x - 1, y) != 1023) << 1;
		hash |= (uint8_t)(getTile(x + 1, y) != 1023) << 2;
		hash |= (uint8_t)(getTile(x, y - 1) != 1023) << 3;
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

	TileWorldDeviceResources deviceResources;

	std::vector<ShaderTypes::LightingUpdate> baseChunkLightingJobs;
	std::vector<ShaderTypes::LightingUpdate> blurChunkLightingJobs;

	void updateBaseLighting();

	void updateBlurLighting();

	void updateLighing() {
		updateBaseLighting();
		updateBlurLighting();
	};

	std::vector<ShaderTypes::LightingUpdate> getBaseLightingUpdateData() {
		ZoneScoped;
		return baseChunkLightingJobs;
	}

	std::vector<ShaderTypes::LightingUpdate> getBlurLightingUpdateData() {
		ZoneScoped;
		return blurChunkLightingJobs;
	}

	void stageChunkUpdates(vk::CommandBuffer commandBuffer);



	struct worldTile_ssbo {
		tileID index;
	};

private:

	void createWorldBuffer() {
		// create foreground and background VRAM buffers

		engine->CreateDeviceOnlyStorageBuffer(sizeof(worldTile_ssbo) * (mapCount), true, deviceResources.MapFGBuffer);
		engine->CreateDeviceOnlyStorageBuffer(sizeof(worldTile_ssbo) * (mapCount), true, deviceResources.MapBGBuffer);
		engine->CreateDeviceOnlyStorageBuffer(sizeof(worldTile_ssbo) * (mapCount) * 4, true, deviceResources.MapLightUpscaleBuffer);
		engine->CreateDeviceOnlyStorageBuffer(sizeof(worldTile_ssbo) * (mapCount) * 4, true, deviceResources.MapLightBlurBuffer);

		/*MapFGBuffer.size = sizeof(worldTile_ssbo) * (mapCount);
		MapBGBuffer.size = sizeof(worldTile_ssbo) * (mapCount);
		MapLightUpscaleBuffer.size = sizeof(worldTile_ssbo) * (mapCount * 4);
		MapLightBlurBuffer.size = sizeof(worldTile_ssbo) * (mapCount * 4);

		engine->createBuffer(MapFGBuffer.size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, MapFGBuffer.buffer, MapFGBuffer.allocation, true);
		engine->createBuffer(MapBGBuffer.size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, MapBGBuffer.buffer, MapBGBuffer.allocation, true);
		
		engine->createBuffer(MapLightUpscaleBuffer.size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, MapLightUpscaleBuffer.buffer, MapLightUpscaleBuffer.allocation, true);
		engine->createBuffer(MapLightBlurBuffer.size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, MapLightBlurBuffer.buffer, MapLightBlurBuffer.allocation, true);*/
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
};

using LargeTileWorld = TileWorld<LargeTileWorldWidth, LargeTileWorldHeight>;
using SmallTileWorld = TileWorld<SmallTileWorldWidth, SmallTileWorldHeight>;