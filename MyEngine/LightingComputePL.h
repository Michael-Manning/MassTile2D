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

	LightingComputePL(std::shared_ptr<VKEngine>& engine, std::shared_ptr<TileWorld> world) : Pipeline(engine), world(world) {
	}

	void createStagingBuffers();

	void CreateComputePipeline(std::string computeSrc_firstPass, std::string computeSrc_secondPass);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, int chunkUpdateCount);

	void stageLightingUpdate(std::vector<chunkLightingUpdateinfo>& chunkUpdates) {
		ZoneScoped;
		if (chunkUpdates.size() == 0)
			return;
		memcpy(lightPositionsDB.buffersMapped[engine->currentFrame], chunkUpdates.data(), sizeof(chunkLightingUpdateinfo) * chunkUpdates.size());
	}

private:
	
	VkPipeline firstStagePipeline;
	VkPipeline secondStagePipeline;

	MappedDoubleBuffer<void> lightPositionsDB;

	std::shared_ptr<TileWorld> world = nullptr;
};