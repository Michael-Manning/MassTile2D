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


class BoidsComputePL :public  Pipeline {

public:

	BoidsComputePL(std::shared_ptr<VKEngine>& engine, std::shared_ptr<TileWorld> world) : Pipeline(engine), world(world) {
	}

	void createStagingBuffers();

	void CreateComputePipeline(const std::vector<uint8_t>& computeSrc_firstPass);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int chunkUpdateCount);


private:

	vk::Pipeline firstStagePipeline;
	vk::Pipeline secondStagePipeline;

	MappedDoubleBuffer<void> lightPositionsDB;

	std::shared_ptr<TileWorld> world = nullptr;
};