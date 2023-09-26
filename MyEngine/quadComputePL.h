#pragma once


#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <string>
#include <utility>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>
#include <tracy/Tracy.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "vertex.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"


class QuadComputePL :public  Pipeline {

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

	QuadComputePL(std::shared_ptr<VKEngine>& engine, int maxInstanceCount) : Pipeline(engine), maxInstanceCount(maxInstanceCount) {
	}

	void allocateTransformBuffer(VkBuffer& buffer, VmaAllocation& allocation);
	void CreateStagingBuffers();

	void UploadInstanceData(std::vector<InstanceBufferData>& drawlist);

	void CreateComputePipeline(const std::vector<uint8_t>& computeSrc, MappedDoubleBuffer<>& cameradb, VkBuffer transformBuffer);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, int instanceCount);


private:

	int maxInstanceCount;
	MappedDoubleBuffer<> ssboMappedDB;
};