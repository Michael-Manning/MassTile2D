#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"
#include "descriptorManager.h"
#include "GlobalImageDescriptor.h"


struct PushConstantInfo {
	uint32_t pushConstantSize = 0;
	vk::ShaderStageFlags pushConstantShaderStages;
};

struct ShaderResourceConfig {
	std::vector<uint8_t> fragmentSrc;
	std::vector<uint8_t> vertexSrc;
	std::vector<std::vector<uint8_t>> computeSrcStages;
	std::vector<DescriptorManager::descriptorSetInfo> descriptorInfos;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	bool flipFaces;
	bool transparentFramebuffer;
	vk::RenderPass renderTarget;
	PushConstantInfo pushInfo;
};

// keyed by set number
using descriptorLayoutMap = std::unordered_map<int, vk::DescriptorSetLayout>;

class Pipeline {
public:

	Pipeline(VKEngine* engine) : engine(engine), descriptorManager(engine) {
	}

protected:

	DescriptorManager descriptorManager;

	// for each frame in flight
	std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT> generalDescriptorSets;

	VKEngine* engine = nullptr;

	vk::Pipeline _pipeline;
	vk::PipelineLayout pipelineLayout;

	std::vector<vk::PipelineShaderStageCreateInfo> createGraphicsShaderStages(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc);
	std::vector<vk::PipelineShaderStageCreateInfo> createComputeShaderStages(const std::vector<std::vector<uint8_t>>& computeSrcs);

	vk::PipelineInputAssemblyStateCreateInfo defaultInputAssembly();
	vk::PipelineViewportStateCreateInfo defaultViewportState();
	vk::PipelineRasterizationStateCreateInfo defaultRasterizer();
	vk::PipelineMultisampleStateCreateInfo defaultMultisampling();
	vk::PipelineColorBlendAttachmentState defaultColorBlendAttachment(bool blendEnabled, bool transparentFramebuffer = false);
	vk::PipelineColorBlendStateCreateInfo defaultColorBlending(vk::PipelineColorBlendAttachmentState *attachment);
	vk::PipelineDynamicStateCreateInfo defaultDynamicState();

	std::vector<vk::DynamicState> defaultDynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	inline vk::Viewport fullframeViewport() {
		vk::Viewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)engine->swapChainExtent.width;
		viewport.height = (float)engine->swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		return viewport;
	};

	void buildPipelineLayout(descriptorLayoutMap& descriptorSetLayouts, uint32_t pushConstantSize = 0, vk::ShaderStageFlags pushConstantStages = vk::ShaderStageFlagBits::eFragment);
};
