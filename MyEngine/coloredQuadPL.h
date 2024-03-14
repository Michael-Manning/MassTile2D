#pragma once

#include <stdint.h>
#include <string>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Vertex.h"
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

	ColoredQuadPL(VKEngine* engine) : pipeline(engine), engine(engine) { }

	void CreateGraphicsPipeline(const PipelineParameters& params, int maxInstances) {
		engine->createMappedBuffer(sizeof(InstanceBufferData) * maxInstances, vk::BufferUsageFlagBits::eStorageBuffer, instanceDataDB);

		PipelineResourceConfig con;
		con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, &params.cameradb.buffers, params.cameradb.size));
		con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(1, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, &instanceDataDB.buffers, instanceDataDB.size));

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

private:
	MappedDoubleBuffer<InstanceBufferData> instanceDataDB;
	GraphicsTemplate pipeline;
	VKEngine* engine;
};
