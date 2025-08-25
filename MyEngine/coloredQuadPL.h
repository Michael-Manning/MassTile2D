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

class ColoredQuadPL {
public:

	struct InstanceBufferData {
		glm::vec4 color;
		alignas(8)glm::vec2 position;
		alignas(8)glm::vec2 scale;
		int circle;
		float rotation;

		int32_t padding[2];
	};
	static_assert(sizeof(InstanceBufferData) % 16 == 0);

	ColoredQuadPL(VKEngine* engine, int maxInstances) 
		: pipeline(engine), engine(engine), maxInstances(maxInstances) { }

	void CreateGraphicsPipeline(const PipelineParameters& params) {
		engine->createMappedBuffer(sizeof(InstanceBufferData) * maxInstances, vk::BufferUsageFlagBits::eStorageBuffer, instanceDataDB);

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

	InstanceBufferData* getUploadMappedBuffer() {
		return instanceDataDB.buffersMapped[engine->currentFrame];
	}

	const int maxInstances;

private:
	MappedDoubleBuffer<InstanceBufferData> instanceDataDB;
	GraphicsTemplate pipeline;
	VKEngine* engine;
};
