#include "stdafx.h"
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

#include "TileWorld.h"

void LargeTileWorld::stageChunkUpdates(vk::CommandBuffer commandBuffer) {

	// nothing updated
	if (tileDirtyState.dirtyIndexes.size() == 0)
		return;

	std::vector<vk::BufferCopy> copyRegions;
	copyRegions.reserve(maxChunkTransfersPerFrame);

	int chunk;
	while (tileDirtyState.dirtyIndexes.pop(chunk)) {
		assert(tileDirtyState.flags[chunk] == ChunkState::Dirty);
		tileDirtyState.flags[chunk] = ChunkState::Clean;

		//std::cout << "transfering chunk " << i << "\n";

		memcpy(chunkTransferBuffers.buffersMapped[engine->currentFrame] + copyRegions.size() * chunkTileCount,
			mapData.data() + chunk * chunkTileCount, sizeof(worldTile_ssbo) * chunkTileCount);

		size_t dstOffset = chunk * chunkTileCount;

		vk::BufferCopy& copyRegion = copyRegions.emplace_back();
		copyRegion.size = chunkTileCount * sizeof(worldTile_ssbo);
		copyRegion.dstOffset = dstOffset * sizeof(worldTile_ssbo);
		copyRegion.srcOffset = (copyRegions.size() - 1) * chunkTileCount * sizeof(worldTile_ssbo);


		if (copyRegions.size() == maxChunkTransfersPerFrame)
			break;
	}

	assert(copyRegions.size() > 0);

	commandBuffer.copyBuffer(chunkTransferBuffers.buffers[engine->currentFrame], deviceResources.MapFGBuffer.buffer, copyRegions.size(), copyRegions.data());

	//must insure transfer is complete before lighting compute shader can access the world buffer
	vk::BufferMemoryBarrier barrier;
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = deviceResources.MapFGBuffer.buffer;
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
};


void LargeTileWorld::updateBlurLighting(){
	ZoneScoped;

	// nothing updated
	if (blurLightingDirtyState.dirtyIndexes.size() == 0)
		return;

	if (blurChunkLightingJobs.size() >= maxChunkBaseLightingUpdatesPerFrame)
		return;

	// could devide each chunk update into quadrents which only check the torches in four surrounding chunks instead of all 9 surrounding chunks
	while (true) {

		if (blurLightingDirtyState.dirtyIndexes.size() == 0)
			break;

		int chunk = blurLightingDirtyState.dirtyIndexes.peek();
		assert(blurLightingDirtyState.flags[chunk] == ChunkState::Dirty);

		uint32_t chunkX = chunk % chunksX;
		uint32_t chunkY = chunk / chunksX;


		// TODO: disregard chunks around borders instead of wrapping around
		bool surroundingOK = true;
		int t = 0;
		for (int i = -1; i < 2; i++) {
			for (int j = -1; j < 2; j++) {
				uint32_t cx = chunkX + i;
				uint32_t cy = chunkY + j;
				uint32_t chunkSearch = cy * chunksX + cx;
				chunkSearch %= chunkCount;
				
				surroundingOK &= baseLightingDirtyState.flags[chunkSearch] == ChunkState::Clean;
			}
		}

		if (surroundingOK == false)
			break;

		blurLightingDirtyState.dirtyIndexes.pop(chunk);

		blurLightingDirtyState.flags[chunk] = ChunkState::Clean;

		ShaderTypes::LightingUpdate update;
		update.chunkIndex = chunk;
		update.lightCount = 0;
	
		blurChunkLightingJobs.push_back(update);

		assert(blurChunkLightingJobs.size() <= maxChunkBaseLightingUpdatesPerFrame);

		if (blurChunkLightingJobs.size() >= maxChunkBaseLightingUpdatesPerFrame)
			return;
	}
}

void LargeTileWorld::updateBaseLighting()
{
	ZoneScoped;

	// nothing updated
	if (baseLightingDirtyState.dirtyIndexes.size() == 0)
		return;

	if (baseChunkLightingJobs.size() >= maxChunkBaseLightingUpdatesPerFrame)
		return;

	// TODO: peek and ensure tile update is not pending before queueing base lighting update

	// could devide each chunk update into quadrents which only check the torches in four surrounding chunks instead of all 9 surrounding chunks
	int chunk;
	while (baseLightingDirtyState.dirtyIndexes.pop(chunk)) {

		assert(baseLightingDirtyState.flags[chunk] == ChunkState::Dirty);
		baseLightingDirtyState.flags[chunk] = ChunkState::Clean;

		uint32_t baseIndex = chunk * chunkTileCount;

		//std::vector<glm::vec2>* searchTorches[9];
		uint32_t chunkX = chunk % chunksX;
		uint32_t chunkY = chunk / chunksX;

		ShaderTypes::LightingUpdate update;
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
					update.lightPositions[t++].v2 = v;
				}
				/*std::copy(torchPositions[chunkSearch].begin(), torchPositions[chunkSearch].end(), update.lightPositions + t);
				t += torchPositions[chunkSearch].size();*/
			}
		}
		if (useMovingTorch) {
			update.lightPositions[t++].v2 = movingTorch;
		}
		update.lightCount = t;
		//std::cout << "light update chunk " << chunk << "\n";
		baseChunkLightingJobs.push_back(update);

		assert(baseChunkLightingJobs.size() <= maxChunkBaseLightingUpdatesPerFrame);

		if (baseChunkLightingJobs.size() >= maxChunkBaseLightingUpdatesPerFrame)
			return;
	}
}