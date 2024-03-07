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
#include <tracy/Tracy.hpp>

#include "texture.h"
#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "vertex.h"
#include "TileWorld.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"


class LightingComputePL :public  Pipeline {

public:

	LightingComputePL(VKEngine* engine, TileWorld* world) : Pipeline(engine), world(world) {
	}

	//void createStagingBuffers();

	void CreateComputePipeline(const std::vector<uint8_t>& computeSrc_firstPass, const std::vector<uint8_t>& computeSrc_secondPass);
	//void CreateComputePipeline(std::string computeSrc_firstPass, std::string computeSrc_secondPass);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int chunkUpdateCount);

	void stageLightingUpdate(std::vector<chunkLightingUpdateinfo>& chunkUpdates) {
		ZoneScoped;
		if (chunkUpdates.size() == 0)
			return;
		std::copy(chunkUpdates.begin(), chunkUpdates.end(), baseLightUpdateDB.buffersMapped[engine->currentFrame]);
		//memcpy(lightPositionsDB.buffersMapped[engine->currentFrame], chunkUpdates.data(), sizeof(chunkLightingUpdateinfo) * chunkUpdates.size());
	}

private:

	vk::Pipeline firstStagePipeline;
	vk::Pipeline secondStagePipeline;

	MappedDoubleBuffer<chunkLightingUpdateinfo> baseLightUpdateDB;
	MappedDoubleBuffer<chunkLightingUpdateinfo> blurLightUpdateDB;

	TileWorld* world = nullptr;
};