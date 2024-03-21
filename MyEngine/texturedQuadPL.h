#pragma once

#include <vector>
#include <stdint.h>
#include <array>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "GlobalImageDescriptor.h"
#include "globalBufferDefinitions.h"
#include "GraphicsTemplate.h"

class TexturedQuadPL {
public:

	struct ssboObjectInstanceData {

		alignas(8) glm::vec2 uvMin;
		alignas(8) glm::vec2 uvMax;

		alignas(8) glm::vec2 translation;
		alignas(8) glm::vec2 scale;
		float rotation = 0.0f;
		int useLightMap = 0;

		texID tex;

		int32_t padding[0];
	};
	static_assert(sizeof(ssboObjectInstanceData) % 16 == 0);

	TexturedQuadPL(VKEngine* engine, int maxInstances)
		: pipeline(engine), engine(engine), maxInstances(maxInstances) { }

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, bool lightMapEnabled, std::array<int, 2> lightMapTextureIndexes = { 0, 0 }) {

		engine->createMappedBuffer(sizeof(ssboObjectInstanceData) * maxInstances, vk::BufferUsageFlagBits::eStorageBuffer, ssboMappedDB);

		engine->createMappedBuffer(vk::BufferUsageFlagBits::eUniformBuffer, lightmapIndexDB);
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
			lightmapIndexDB.buffersMapped[i]->lightMapIndex = lightMapTextureIndexes[i];

		PipelineResourceConfig con;
		con.bufferBindings.push_back(BufferBinding(1, 1, params.cameraDB));
		con.bufferBindings.push_back(BufferBinding(1, 0, ssboMappedDB));
		con.bufferBindings.push_back(BufferBinding(1, 2, lightmapIndexDB));

		con.specConstBindings.push_back(SpecConstantBinding{ 0, static_cast<uint32_t>(lightMapEnabled ? 1: 0) });

		con.globalDescriptors.push_back({ 0, textureDescriptor });

		pipeline.CreateGraphicsPipeline(params, con);
	}

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount) {
		TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "textured quad render");

		assert(instanceCount > 0);

		pipeline.bindPipelineResources(commandBuffer);

		commandBuffer.drawIndexed(QuadIndices.size(), instanceCount, 0, 0, 0);
	}

	ssboObjectInstanceData* getUploadMappedBuffer() {
		return ssboMappedDB.buffersMapped[engine->currentFrame];
	}

	const int maxInstances;

private:

	struct lightMapIndex_UBO {
		int lightMapIndex;
	};

	VKEngine* engine;
	GraphicsTemplate pipeline;

	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<ssboObjectInstanceData> ssboMappedDB;
	MappedDoubleBuffer<lightMapIndex_UBO> lightmapIndexDB;
};
