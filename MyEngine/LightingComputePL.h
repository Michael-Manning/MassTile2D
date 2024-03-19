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

#include <tracy/Tracy.hpp>

#include "texture.h"
#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "vertex.h"
#include "TileWorld.h"
#include "vulkan_util.h"
#include "ComputeTemplate.h"
#include "globalBufferDefinitions.h"


class LightingComputePL {

public:

	LightingComputePL(VKEngine* engine, TileWorld* world) : engine(engine), world(world), pipelines(engine) {
	}

	void CreateComputePipeline(const std::vector<uint8_t>& computeSrc_firstPass, const std::vector<uint8_t>& computeSrc_secondPass, const std::vector<uint8_t>& computeSrc_thirdPass);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int baseUpdates, int blurUpdates);

	void stageLightingUpdate(std::vector<chunkLightingUpdateinfo>& baseUpdates, std::vector<chunkLightingUpdateinfo>& blurUpdates) {
		ZoneScoped;
		if (baseUpdates.size() != 0)
			std::copy(baseUpdates.begin(), baseUpdates.end(), baseLightUpdateDB.buffersMapped[engine->currentFrame]);

		if (blurUpdates.size() != 0)
			std::copy(blurUpdates.begin(), blurUpdates.end(), blurLightUpdateDB.buffersMapped[engine->currentFrame]);
	}

private:

	VKEngine* engine;

	ComputeTemplate pipelines;

	MappedDoubleBuffer<chunkLightingUpdateinfo> baseLightUpdateDB;
	MappedDoubleBuffer<chunkLightingUpdateinfo> blurLightUpdateDB;

	TileWorld* world = nullptr;
};