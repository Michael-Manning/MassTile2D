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

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vk_mem_alloc.h>

//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VKEngine.h"
#include "pipeline.h"

#include "vulkan_util.h"

using namespace std;

std::vector<VkPipelineShaderStageCreateInfo> Pipeline::createShaderStages(std::string vertexSrc, std::string fragmentSrc) {
	auto vertShaderCode = VKUtil::readFile(vertexSrc);
	auto fragShaderCode = VKUtil::readFile(fragmentSrc);

	VkShaderModule vertShaderModule = VKUtil::createShaderModule(vertShaderCode, engine->device);
	VkShaderModule fragShaderModule = VKUtil::createShaderModule(fragShaderCode, engine->device);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	return { vertShaderStageInfo, fragShaderStageInfo };
}

VkPipelineInputAssemblyStateCreateInfo Pipeline::defaultInputAssembly() {
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	return inputAssembly;
}

VkPipelineViewportStateCreateInfo Pipeline::defaultViewportState() {
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	return viewportState;
}

VkPipelineRasterizationStateCreateInfo Pipeline::defaultRasterizer() {
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	return rasterizer;
}

VkPipelineMultisampleStateCreateInfo Pipeline::defaultMultisampling() {
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	return multisampling;
}

VkPipelineColorBlendAttachmentState Pipeline::defaultColorBlendAttachment(bool blendEnabled) {
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	if (blendEnabled) {
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}
	else {
		colorBlendAttachment.blendEnable = VK_FALSE;
	}

	return colorBlendAttachment;
}

VkPipelineColorBlendStateCreateInfo Pipeline::defaultColorBlending(VkPipelineColorBlendAttachmentState* attachment) {
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = attachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	return colorBlending;
}

VkPipelineDynamicStateCreateInfo Pipeline::defaultDynamicState() {
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(defaultDynamicStates.size());
	dynamicState.pDynamicStates = defaultDynamicStates.data();

	return dynamicState;
}


VkDescriptorSetLayoutBinding Pipeline::buildSamplerBinding(int binding, int descriptorCount, VkShaderStageFlags stageFlags) {
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = binding;
	samplerLayoutBinding.descriptorCount = descriptorCount;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = stageFlags;
	return samplerLayoutBinding;
}

VkDescriptorSetLayoutBinding Pipeline::buildUBOBinding(int binding, VkShaderStageFlags stageFlags) {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = binding;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = stageFlags;
	return uboLayoutBinding;
}

VkDescriptorSetLayoutBinding Pipeline::buildSSBOBinding(int binding, VkShaderStageFlags stageFlags) {
	VkDescriptorSetLayoutBinding ssboLayoutBinding{};
	ssboLayoutBinding.binding = binding;
	ssboLayoutBinding.descriptorCount = 1;
	ssboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ssboLayoutBinding.pImmutableSamplers = nullptr;
	ssboLayoutBinding.stageFlags = stageFlags;
	return ssboLayoutBinding;
}

void Pipeline::buildSetLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout& layout) {
	VkDescriptorSetLayoutCreateInfo setInfo = {};
	setInfo.bindingCount = bindings.size();
	setInfo.flags = 0;
	setInfo.pNext = nullptr;
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.pBindings = bindings.data();

	assert(vkCreateDescriptorSetLayout(engine->device, &setInfo, nullptr, &layout) == VK_SUCCESS);
}

void Pipeline::buildPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, uint32_t pushConstantSize, VkShaderStageFlags pushConstantStages) {

	// setup layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = pushConstantSize == 0 ? 0 : 1;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();;

	if (pushConstantSize > 0) {
		VkPushConstantRange push_constant;
		push_constant.offset = 0;
		push_constant.size = pushConstantSize;
		push_constant.stageFlags = pushConstantStages;

		pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	}

	assert(vkCreatePipelineLayout(engine->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) == VK_SUCCESS);
}

void Pipeline::buidDBDescriptorSet(VkDescriptorSetLayout& layout, std::array<VkDescriptorSet, FRAMES_IN_FLIGHT>& sets) {

	std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts = { layout, layout };

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = engine->descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	assert(vkAllocateDescriptorSets(engine->device, &allocInfo, sets.data()) == VK_SUCCESS);
}