#include "stdafx.h"

#include <vector>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <stb_image.h>

#include "VKengine.h"
#include "typedefs.h"
#include "texturedQuadPL.h"
#include "globalBufferDefinitions.h"
#include "GlobalImageDescriptor.h"
#include "GraphicsTemplate.h"

using namespace glm;
using namespace std;


void TexturedQuadPL::CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, std::array<int, 2>& lightMapTextureIndexes) {

	engine->createMappedBuffer(sizeof(ssboObjectInstanceData) * TexturedQuadPL_MAX_OBJECTS, vk::BufferUsageFlagBits::eStorageBuffer, ssboMappedDB);

	engine->createMappedBuffer(vk::BufferUsageFlagBits::eUniformBuffer, lightmapIndexDB);
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		lightmapIndexDB.buffersMapped[i]->lightMapIndex = lightMapTextureIndexes[i];

	PipelineResourceConfig con;
	con.bufferBindings.push_back(BufferBinding(1, 1, params.cameraDB));
	con.bufferBindings.push_back(BufferBinding(1, 0, ssboMappedDB));
	con.bufferBindings.push_back(BufferBinding(1, 2, lightmapIndexDB));

	con.globalDescriptors.push_back({ 0, textureDescriptor });

	pipeline.CreateGraphicsPipeline(params, con);
}


void TexturedQuadPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount) {
	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "textured quad render");

	assert(instanceCount > 0);

	pipeline.bindPipelineResources(commandBuffer);

	commandBuffer.drawIndexed(QuadIndices.size(), instanceCount, 0, 0, 0);
}