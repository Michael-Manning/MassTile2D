#pragma once

#include <vector>
#include <stdint.h>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "TileWorld.h"
#include "globalBufferDefinitions.h"
#include "GraphicsTemplate.h"

class TilemapLightRasterPL{
public:

	TilemapLightRasterPL(VKEngine* engine, TileWorld* world) : pipeline(engine), engine(engine), world(world) {
	}

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex);

private:
	TileWorld* world = nullptr;

	GlobalImageDescriptor* textureDescriptor;
	GraphicsTemplate pipeline;

	VKEngine* engine;
};
