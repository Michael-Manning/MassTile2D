#pragma once

#include <stdint.h>
#include <string>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Vertex2D.h"
#include "GraphicsTemplate.h"
#include "globalBufferDefinitions.h"
#include "ShaderTypes.h"

class ColoredQuadPL {
public:

	ColoredQuadPL(VKEngine* engine, int maxInstances) 
		: pipeline(engine), engine(engine), maxInstances(maxInstances) { }

	void CreateGraphicsPipeline(const PipelineParameters& params) {
		engine->createMappedBuffer(sizeof(ShaderTypes::ColoredQuadInstance) * maxInstances, vk::BufferUsageFlagBits::eStorageBuffer, instanceDataDB);

		PipelineResourceConfig con;
		con.bufferBindings.push_back(BufferBinding(0, 0, params.cameraDB));
		con.bufferBindings.push_back(BufferBinding(1, 0, instanceDataDB));

		pipeline.CreateGraphicsPipeline(params, con);
	}

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount) {
		TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "Colored quad render");

		pipeline.bindPipelineResources(commandBuffer);
		commandBuffer.drawIndexed(QuadIndices.size(), instanceCount, 0, 0, 0);
	}

	ShaderTypes::ColoredQuadInstance* getUploadMappedBuffer() {
		return instanceDataDB.buffersMapped[engine->currentFrame]->instanceData;
	}

	const int maxInstances;

private:
	MappedDoubleBuffer<ShaderTypes::ColoredQuadInstaceBuffer> instanceDataDB;
	GraphicsTemplate pipeline;
	VKEngine* engine;
};
