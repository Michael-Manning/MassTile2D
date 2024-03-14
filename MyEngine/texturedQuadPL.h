#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <string>
#include <array>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "BindingManager.h"
#include "GlobalImageDescriptor.h"
#include "globalBufferDefinitions.h"

constexpr int TexturedQuadPL_MAX_OBJECTS = 100000;


class TexturedQuadPL :public  Pipeline {
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

	TexturedQuadPL(VKEngine* engine) : 
		Pipeline(engine) { }

	void CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, GlobalImageDescriptor* textureDescriptor, MappedDoubleBuffer<coodinateTransformUBO_s>& cameradb, std::array<int, 2>&lightMapTextureIndexes, bool flipFaces = false);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount);

	ssboObjectInstanceData* getUploadMappedBuffer() {
		return ssboMappedDB.buffersMapped[engine->currentFrame];
	}

private:

	struct lightMapIndex_UBO {
		int lightMapIndex;
	};

	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<ssboObjectInstanceData> ssboMappedDB;
	MappedDoubleBuffer<lightMapIndex_UBO> lightmapIndexDB;
};
