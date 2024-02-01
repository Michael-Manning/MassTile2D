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

#include <vk_mem_alloc.h>

//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VKEngine.h"
#include "pipeline.h"

#include "vulkan_util.h"
#include "Utils.h"

using namespace std;

std::vector<vk::PipelineShaderStageCreateInfo> Pipeline::createShaderStages(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc) {
	/*auto vertShaderCode = readFile(vertexSrc);
	auto fragShaderCode = readFile(fragmentSrc);*/

	/*vk::ShaderModule vertShaderModule = VKUtil::createShaderModule(vertShaderCode, engine->devContext.device);
	vk::ShaderModule fragShaderModule = VKUtil::createShaderModule(fragShaderCode, engine->devContext.device);*/

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

vk::PipelineShaderStageCreateInfo Pipeline::createComputeShaderStage(const std::vector<uint8_t>& shaderCode) {
	//auto shaderCode = readFile(computeSrc);

	vk::ShaderModule vertShaderModule = VKUtil::createShaderModule(shaderCode, engine->devContext.device);

	vk::PipelineShaderStageCreateInfo stageInfo{};
	stageInfo.stage = vk::ShaderStageFlagBits::eCompute;
	stageInfo.module = vertShaderModule;
	stageInfo.pName = "main";

	return stageInfo;
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
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
		if (transparentFramebuffer) {
			colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
			colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		}
		else {
			colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eZero;
			colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOne;
		}
		colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
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


//vk::DescriptorSetLayoutBinding Pipeline::buildSamplerBinding(int binding, int descriptorCount, vk::ShaderStageFlags stageFlags) {
//	vk::DescriptorSetLayoutBinding samplerLayoutBinding;
//	samplerLayoutBinding.binding = binding;
//	samplerLayoutBinding.descriptorCount = descriptorCount;
//	samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
//	samplerLayoutBinding.pImmutableSamplers = nullptr;
//	samplerLayoutBinding.stageFlags = stageFlags;
//	return samplerLayoutBinding;
//}
//
//vk::DescriptorSetLayoutBinding Pipeline::buildUBOBinding(int binding, vk::ShaderStageFlags stageFlags) {
//	vk::DescriptorSetLayoutBinding uboLayoutBinding;
//	uboLayoutBinding.binding = binding;
//	uboLayoutBinding.descriptorCount = 1;
//	uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
//	uboLayoutBinding.pImmutableSamplers = nullptr;
//	uboLayoutBinding.stageFlags = stageFlags;
//	return uboLayoutBinding;
//}
//
//vk::DescriptorSetLayoutBinding Pipeline::buildSSBOBinding(int binding, vk::ShaderStageFlags stageFlags) {
//	vk::DescriptorSetLayoutBinding ssboLayoutBinding;
//	ssboLayoutBinding.binding = binding;
//	ssboLayoutBinding.descriptorCount = 1;
//	ssboLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
//	ssboLayoutBinding.pImmutableSamplers = nullptr;
//	ssboLayoutBinding.stageFlags = stageFlags;
//	return ssboLayoutBinding;
//}
//
//void Pipeline::buildSetLayout(std::vector<vk::DescriptorSetLayoutBinding>& bindings, vk::DescriptorSetLayout& layout) {
//	vk::DescriptorSetLayoutCreateInfo setInfo;
//	setInfo.bindingCount = bindings.size();
//	setInfo.flags = vk::DescriptorSetLayoutCreateFlags{};
//	setInfo.pNext = nullptr;
//	setInfo.pBindings = bindings.data();
//
//	layout = engine->devContext.device.createDescriptorSetLayout(setInfo);
//}

void Pipeline::buildPipelineLayout(descriptorLayoutMap& descriptorSetLayouts, uint32_t pushConstantSize, vk::ShaderStageFlags pushConstantStages) {

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

//void Pipeline::buidDBDescriptorSet(vk::DescriptorSetLayout& layout, std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT>& sets) {
//
//	std::array<vk::DescriptorSetLayout, FRAMES_IN_FLIGHT> layouts = { layout, layout };
//
//	vk::DescriptorSetAllocateInfo allocInfo;
//	allocInfo.descriptorPool = engine->descriptorPool;
//	allocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
//	allocInfo.pSetLayouts = layouts.data();
//
//	auto dsV = engine->devContext.device.allocateDescriptorSets(allocInfo);
//	for (size_t i = 0; i < sets.size(); i++) {
//		sets[i] = dsV[i];
//	}
//}
//
//
//void Pipeline::buildDescriptorLayouts() {
//
//	// a layout binding for each set
//	unordered_map<int, vector<vk::DescriptorSetLayoutBinding>> builderLayoutBindings;
//
//	for (auto& i : builderDescriptorSetsDetails) {
//
//		vk::DescriptorSetLayoutBinding binding;
//
//		switch (i.type)
//		{
//		case vk::DescriptorType::eUniformBuffer:
//			binding = buildUBOBinding(i.binding, i.stageFlags);
//			break;
//		case vk::DescriptorType::eStorageBuffer:
//			binding = buildSSBOBinding(i.binding, i.stageFlags);
//			break;
//		case vk::DescriptorType::eCombinedImageSampler:
//			binding = buildSamplerBinding(i.binding, i.textureCount, i.stageFlags);
//			break;
//		default:
//			assert(false);
//			break;
//		}
//
//		builderLayoutBindings[i.set].push_back(binding);
//	}
//
//	// iterate builderLayoBindings and build the actual layouts
//	for (auto& [set, binding] : builderLayoutBindings) {
//		vk::DescriptorSetLayout layout;
//		buildSetLayout(binding, layout);
//		builderLayouts[set] = layout;
//	}
//}
//void Pipeline::buildDescriptorSets() {
//
//
//	for (auto& [set, layout] : builderLayouts) {
//		buidDBDescriptorSet(layout, builderDescriptorSets[set]);
//	}
//
//	// one time write to every descriptor set
//	for (auto& info : builderDescriptorSetsDetails)
//	{
//		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
//
//			updateDescriptorSet(i, info);
//		}
//	}
//}
//
//void Pipeline::updateDescriptorSet(int frame, descriptorSetInfo& info) {
//	vk::WriteDescriptorSet descriptorWrite;
//	descriptorWrite.dstSet = builderDescriptorSets[info.set][frame];
//	descriptorWrite.dstBinding = info.binding;
//	descriptorWrite.dstArrayElement = 0;
//	descriptorWrite.descriptorType = info.type;
//
//	if (info.type == vk::DescriptorType::eUniformBuffer || info.type == vk::DescriptorType::eStorageBuffer) {
//		assert(info.doubleBuffer != nullptr);
//		assert(info.bufferRange != 0);
//
//		vk::DescriptorBufferInfo bufferInfo{};
//		bufferInfo.buffer = (*info.doubleBuffer)[frame];
//		bufferInfo.offset = 0;
//		bufferInfo.range = info.bufferRange;
//		descriptorWrite.descriptorCount = 1;
//		descriptorWrite.pBufferInfo = &bufferInfo;
//
//		engine->devContext.device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
//	}
//	else if (info.type == vk::DescriptorType::eCombinedImageSampler) {
//
//		assert(info.textures != nullptr);
//
//		vector<vk::DescriptorImageInfo> imageInfos(info.textureCount);
//		for (size_t i = 0; i < info.textureCount; i++) {
//			vk::DescriptorImageInfo imageInfo{};
//			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
//			imageInfo.imageView = info.textures[i].imageView;
//			imageInfo.sampler = info.textures[i].sampler;
//			imageInfos[i] = imageInfo;
//		}
//
//		descriptorWrite.descriptorCount = info.textureCount;
//		descriptorWrite.pImageInfo = imageInfos.data();
//
//		engine->devContext.device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
//	}
//	else {
//		assert(false);
//	}
//}