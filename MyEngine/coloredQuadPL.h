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

constexpr int ColoredQuadPL_MAX_OBJECTS = 100000;

class ColoredQuadPL :public  Pipeline {
public:

	struct ssboObjectInstanceData {

		glm::vec4 color;
		alignas(8)glm::vec2 position;
		alignas(8)glm::vec2 scale;
		int circle;
		float rotation;

		int32_t padding[2];
	};

	static_assert(sizeof(ssboObjectInstanceData) % 16 == 0);

	//struct DrawItem {
	//	glm::vec4 color;
	//	glm::vec2 position;
	//	glm::vec2 scale;
	//	int circle;
	//	float rotation;
	//};

	ColoredQuadPL(std::shared_ptr<VKEngine>& engine, VertexMeshBuffer quadMesh) : Pipeline(engine), quadMesh(quadMesh) {
	}

	void CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) override;
	void createDescriptorSetLayout() override;
	void createDescriptorSets(MappedDoubleBuffer& cameradb);
	void createSSBOBuffer();

	void recordCommandBuffer(VkCommandBuffer commandBuffer, std::vector<ssboObjectInstanceData>& drawlist);

private:
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout SSBOSetLayout;

	VertexMeshBuffer quadMesh;

	MappedDoubleBuffer ssboMappedDB;
};
