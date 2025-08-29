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

	TilemapLightRasterPL(VKEngine* engine) : pipeline(engine), engine(engine) {
	}

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, TileWorldDeviceResources* tileWorldData);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex, const ShaderTypes::LightingSettings& lightingSettings);

private:
	TileWorldDeviceResources* tileWorldData = nullptr;

	GlobalImageDescriptor* textureDescriptor;
	GraphicsTemplate pipeline;

	VKEngine* engine;
};
