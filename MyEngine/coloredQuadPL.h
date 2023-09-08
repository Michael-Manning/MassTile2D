#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <utility>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"

constexpr int ColoredQuadPL_MAX_OBJECTS = 1000000;




#include "quadComputePL.h"


class ColoredQuadPL :public  Pipeline {
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

	ColoredQuadPL(std::shared_ptr<VKEngine>& engine, VertexMeshBuffer quadMesh) : Pipeline(engine), quadMesh(quadMesh) {
	}

	void SetTransformBuffer(VkBuffer transformBuffer) {
		this->transformBuffer = transformBuffer;
	};

	void temp(std::vector<QuadComputePL::InstanceBufferData>& drawlist) {
		memcpy(ssboMappedDB.buffersMapped[engine->currentFrame], drawlist.data(), sizeof(InstanceBufferData) * drawlist.size());
	};

	void CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc, MappedDoubleBuffer& cameradb);
	void CreateInstancingBuffer();
	void UploadInstanceData(std::vector<InstanceBufferData>& drawlist);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, int instanceCount);

private:

	VertexMeshBuffer quadMesh;
	MappedDoubleBuffer ssboMappedDB;

	VkBuffer transformBuffer;
};
