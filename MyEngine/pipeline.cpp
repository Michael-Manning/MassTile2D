#include "stdafx.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <fstream>
#include <chrono>
#include <cassert>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vma/vk_mem_alloc.h>

#include <stb_image.h>

#include "VKEngine.h"
#include "pipeline.h"

#include "vulkan_util.h"
#include "Utils.h"

using namespace std;

std::vector<vk::PipelineShaderStageCreateInfo> Pipeline::createGraphicsShaderStages(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc) {

	vk::ShaderModule vertShaderModule = VKUtil::createShaderModule(vertexSrc, engine->devContext.device);
	vk::ShaderModule fragShaderModule = VKUtil::createShaderModule(fragmentSrc, engine->devContext.device);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
	vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	return { vertShaderStageInfo, fragShaderStageInfo };
}

std::vector<vk::PipelineShaderStageCreateInfo> Pipeline::createComputeShaderStages(const std::vector<std::vector<uint8_t>>& computeSrcs) {

	std::vector<vk::PipelineShaderStageCreateInfo> infos;
	infos.reserve(computeSrcs.size());

	for (auto& src : computeSrcs)
	{
		vk::PipelineShaderStageCreateInfo& stageInfo = infos.emplace_back();
		stageInfo.stage = vk::ShaderStageFlagBits::eCompute;
		stageInfo.module = VKUtil::createShaderModule(src, engine->devContext.device);
		stageInfo.pName = "main";
	}

	return infos;
}

vk::PipelineInputAssemblyStateCreateInfo Pipeline::defaultInputAssembly() {
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	return inputAssembly;
}

vk::PipelineViewportStateCreateInfo Pipeline::defaultViewportState() {
	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	return viewportState;
}

vk::PipelineRasterizationStateCreateInfo Pipeline::defaultRasterizer() {
	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = VK_FALSE;

	return rasterizer;
}

vk::PipelineMultisampleStateCreateInfo Pipeline::defaultMultisampling() {
	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

	return multisampling;
}

vk::PipelineColorBlendAttachmentState Pipeline::defaultColorBlendAttachment(bool blendEnabled, bool transparentFramebuffer) {
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eR;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eG;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eB;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eA;

	if (blendEnabled) {

		if (transparentFramebuffer) {
			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
			colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
			colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
			colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOne;
			colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
		}
		else {
			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
			colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
			colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eZero;
			colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOne;
			colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
		}

	}
	else {
		colorBlendAttachment.blendEnable = VK_FALSE;
	}

	return colorBlendAttachment;
}

vk::PipelineColorBlendStateCreateInfo Pipeline::defaultColorBlending(vk::PipelineColorBlendAttachmentState* attachment) {
	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = attachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	return colorBlending;
}

vk::PipelineDynamicStateCreateInfo Pipeline::defaultDynamicState() {
	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(defaultDynamicStates.size());
	dynamicState.pDynamicStates = defaultDynamicStates.data();
	return dynamicState;
}

void Pipeline::buildPipelineLayout(descriptorLayoutMap& descriptorSetLayouts, uint32_t pushConstantSize, vk::ShaderStageFlags pushConstantStages) {

	// bind indexes are explicit in shaders, but set indexes are determined by their contiguous order in the layout array.
	// This function accepts both the bind and set indexes explicitly, then validates and sorts them for the vulkan structures.

	// ensure set numbers start from zero and don't skip any values
	{
		for (int i = 0; i < descriptorSetLayouts.size(); ++i) {
			assert(descriptorSetLayouts.contains(i));
		}
	}

	// sort layouts by their set ID

	// Step 1: Extract key-value pairs into a vector
	std::vector<std::pair<int, vk::DescriptorSetLayout>> pairs;
	for (const auto& kv : descriptorSetLayouts) {
		pairs.push_back(kv);
	}

	// Step 2: Sort the pairs by key
	std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
		return a.first < b.first;
		});

	// Step 3: Extract the values into a new vector
	std::vector<vk::DescriptorSetLayout> sortedValues;
	for (const auto& pair : pairs) {
		sortedValues.push_back(pair.second);
	}


	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.pushConstantRangeCount = pushConstantSize == 0 ? 0 : 1;
	pipelineLayoutInfo.setLayoutCount = sortedValues.size();
	pipelineLayoutInfo.pSetLayouts = sortedValues.data();

	vk::PushConstantRange push_constant;
	if (pushConstantSize > 0) {
		push_constant.offset = 0;
		push_constant.size = pushConstantSize;
		push_constant.stageFlags = pushConstantStages;

		pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	}

	pipelineLayout = engine->devContext.device.createPipelineLayout(pipelineLayoutInfo);
}
