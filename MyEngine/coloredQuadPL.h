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


class ColoredQuadPL :public  Pipeline {
public:

	struct DrawItem {
		glm::vec4 color;
		glm::vec2 position;
		glm::vec2 scale;
		int circle;
		float rotation;
	};

	ColoredQuadPL(std::shared_ptr<VKEngine>& engine) : Pipeline(engine) {
	}

	void CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) override;
	void createDescriptorSetLayout() override;
	void createDescriptorSets(MappedDoubleBuffer& cameradb);
	void createVertices() override;

	void recordCommandBuffer(VkCommandBuffer commandBuffer, std::vector<DrawItem>& drawlist);

private:

	std::array<bool, FRAMES_IN_FLIGHT> uboDirtyFlags = { true, true };

	VkDescriptorSetLayout descriptorSetLayout;

};