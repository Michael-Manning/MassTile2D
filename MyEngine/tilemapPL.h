#pragma once

#include <vector>
#include <stdint.h>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <tracy/TracyVulkan.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "TileWorld.h"
#include "globalBufferDefinitions.h"
#include "GraphicsTemplate.h"

class TilemapPL{
public:

	TilemapPL(VKEngine* engine) : pipeline(engine), engine(engine) {}

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, TileWorldDeviceResources* tileWorldData) {
		PipelineResourceConfig con;

		con.bufferBindings.push_back(BufferBinding(1, 0, params.cameraDB));
		con.bufferBindings.push_back(BufferBinding(1, 1, tileWorldData->MapFGBuffer));
		con.bufferBindings.push_back(BufferBinding(1, 2, tileWorldData->MapBGBuffer));

		con.globalDescriptors.push_back({ 0, textureDescriptor });

		pipeline.CreateGraphicsPipeline(params, con);
	}

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex, int lightMapIndex) {
		TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "Tilemap render");

		pipeline.bindPipelineResources(commandBuffer);

		pipeline.UpdatePushConstant(commandBuffer, pushConstant_s{
			.textureIndex = textureIndex,
			.lightMapIndex = lightMapIndex
			});

		commandBuffer.drawIndexed(QuadIndices.size(), 1, 0, 0, 0);
	}

private:
	struct pushConstant_s {
		int32_t textureIndex;
		int32_t lightMapIndex;
	};

	VKEngine* engine;
	GraphicsTemplate pipeline;
};
