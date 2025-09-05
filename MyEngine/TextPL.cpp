#include "stdafx.h"

#include <vector>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "VKengine.h"
#include "typedefs.h"
#include "globalBufferDefinitions.h"a
#include "ShaderTypes.h"
#include "ShaderUtility.h"

#include "TextPL.h"

using namespace glm;
using namespace std;

void TextPL::CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor) {

	ShaderUtil::CreateMappedInstanceBuffer(engine, maxObjects, textHeadersDB);
	ShaderUtil::CreateMappedInstanceBuffer(engine, maxObjects * maxStringLength, textQuadsDB);
	ShaderUtil::CreateMappedInstanceBuffer(engine, maxObjects * maxStringLength, letterIndexDB);

	PipelineResourceConfig con = ShaderUtil::text_CreateBufferBindings(
		textHeadersDB, 
		textQuadsDB, 
		params.cameraDB, 
		letterIndexDB, 
		textureDescriptor
	);

	con.globalDescriptors.push_back({ 0, textureDescriptor });

	pipeline.CreateGraphicsPipeline(params, con);
}

void TextPL::recordCommandBuffer(vk::CommandBuffer commandBuffer) {

	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "text render");

	int instanceCount = 0;

	int indexBufferIndex = 0;
	for (int i = 0; i < maxObjects; i++) {
		int objInstances = textHeadersDB.buffersMapped[engine->currentFrame]->headerData[i].textLength;
		instanceCount += objInstances;

		for (size_t j = 0; j < objInstances; j++)
		{
			letterIndexDB.buffersMapped[engine->currentFrame]->indexData[indexBufferIndex].headerIndex = i;
			letterIndexDB.buffersMapped[engine->currentFrame]->indexData[indexBufferIndex].letterIndex = i * maxStringLength + j;
			indexBufferIndex++;
		}
	}

	if (instanceCount == 0)
		return;

	pipeline.bindPipelineResources(commandBuffer);

	commandBuffer.drawIndexed(QuadIndices.size(), instanceCount, 0, 0, 0);
}
