#pragma once


#include <vector>
#include <string>
#include <memory>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"


class Pipeline {
public:

	Pipeline(std::shared_ptr<VKEngine> engine) : engine(engine) {
	}

	// abstract
	virtual void CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) = 0;
	virtual void createDescriptorSetLayout() = 0;
	virtual void createDescriptorSets() = 0;
	virtual void createVertices() = 0;
	void recordCommandBuffer(VkCommandBuffer commandBuffer);

protected:

	// for each frame in flight
	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> generalDescriptorSets;


	std::shared_ptr<VKEngine> engine = nullptr;

	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;


	std::vector<VkBuffer> uniformBuffers;
	std::vector<VmaAllocation> uniformBuffersAllocation;
	std::vector<void*> uniformBuffersMapped;

	//std::vector<VkBuffer> ssboBuffers;
	//std::vector<VmaAllocation> ssboAllocations;
	//std::vector<void*> ssboBuffersMapped;

	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	VkBuffer indexBuffer;
	VmaAllocation indexBufferAllocation;

	std::vector<VkPipelineShaderStageCreateInfo> createShaderStages(std::string vertexSrc, std::string fragmentSrc);

	VkPipelineInputAssemblyStateCreateInfo defaultInputAssembly();
	VkPipelineViewportStateCreateInfo defaultViewportState();
	VkPipelineRasterizationStateCreateInfo defaultRasterizer();
	VkPipelineMultisampleStateCreateInfo defaultMultisampling();
	VkPipelineColorBlendAttachmentState defaultColorBlendAttachment(bool blendEnabled);
	VkPipelineColorBlendStateCreateInfo defaultColorBlending(VkPipelineColorBlendAttachmentState *attachment);
	VkPipelineDynamicStateCreateInfo defaultDynamicState();

	std::vector<VkDynamicState> defaultDynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	inline VkViewport fullframeViewport() {
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)engine->swapChainExtent.width;
		viewport.height = (float)engine->swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		return viewport;
	};

	VkDescriptorSetLayoutBinding buildSamplerBinding(int binding, int descriptorCount, VkShaderStageFlags stageFlags);
	VkDescriptorSetLayoutBinding buildUBOBinding(int binding, VkShaderStageFlags stageFlags);
	VkDescriptorSetLayoutBinding buildSSBOBinding(int binding, VkShaderStageFlags stageFlags);
	void buildSetLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout& layout);

	void buildPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, uint32_t pushConstantSize = 0, VkShaderStageFlags pushConstantStages = 0);

	void buidDBDescriptorSet(VkDescriptorSetLayout& layout, std::array<VkDescriptorSet, FRAMES_IN_FLIGHT>& sets);
};
