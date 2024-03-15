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

constexpr int TexturedQuadPL_MAX_OBJECTS = 100000;


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

	TexturedQuadPL(VKEngine* engine) : pipeline(engine), engine(engine) { }

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, std::array<int, 2>&lightMapTextureIndexes);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount);

	ssboObjectInstanceData* getUploadMappedBuffer() {
		return ssboMappedDB.buffersMapped[engine->currentFrame];
	}

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
