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
#include <stb_image.h>

#include "VKEngine.h"
#include "descriptorManager.h"

#include "vulkan_util.h"
#include "Utils.h"

using namespace std;

vk::DescriptorSetLayoutBinding DescriptorManager::buildSamplerBinding(int binding, int descriptorCount, vk::ShaderStageFlags stageFlags) {
	vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = binding;
	samplerLayoutBinding.descriptorCount = descriptorCount;
	samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = stageFlags;
	return samplerLayoutBinding;
}

vk::DescriptorSetLayoutBinding DescriptorManager::buildUBOBinding(int binding, vk::ShaderStageFlags stageFlags) {
	vk::DescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = binding;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = stageFlags;
	return uboLayoutBinding;
}

vk::DescriptorSetLayoutBinding DescriptorManager::buildSSBOBinding(int binding, vk::ShaderStageFlags stageFlags) {
	vk::DescriptorSetLayoutBinding ssboLayoutBinding{};
	ssboLayoutBinding.binding = binding;
	ssboLayoutBinding.descriptorCount = 1;
	ssboLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
	ssboLayoutBinding.pImmutableSamplers = nullptr;
	ssboLayoutBinding.stageFlags = stageFlags;
	return ssboLayoutBinding;
}

void DescriptorManager::buildSetLayout(std::vector<vk::DescriptorSetLayoutBinding>& bindings, vk::DescriptorSetLayout& layout) {
	vk::DescriptorSetLayoutCreateInfo setInfo;
	setInfo.bindingCount = bindings.size();
	setInfo.pNext = nullptr;
	setInfo.pBindings = bindings.data();

	layout = engine->devContext.device.createDescriptorSetLayout(setInfo);
}

void DescriptorManager::buildDescriptorLayouts() {

	// a layout binding for each set
	unordered_map<int, vector<vk::DescriptorSetLayoutBinding>> builderLayoutBindings;

	for (auto& i : builderDescriptorSetsDetails) {

		VkDescriptorSetLayoutBinding binding;

		switch (i.type)
		{
		case vk::DescriptorType::eUniformBuffer:
			binding = buildUBOBinding(i.binding, i.stageFlags);
			break;
		case vk::DescriptorType::eStorageBuffer:
			binding = buildSSBOBinding(i.binding, i.stageFlags);
			break;
		case vk::DescriptorType::eCombinedImageSampler:
			binding = buildSamplerBinding(i.binding, i.textureCount, i.stageFlags);
			break;
		default:
			assert(false);
			break;
		}

		builderLayoutBindings[i.set].push_back(binding);
	}

	// iterate builderLayoBindings and build the actual layouts
	for (auto& [set, binding] : builderLayoutBindings) {
		vk::DescriptorSetLayout layout;
		buildSetLayout(binding, layout);
		builderLayouts[set] = layout;
	}
}

void DescriptorManager::buidDBDescriptorSet(vk::DescriptorSetLayout& layout, std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT>& sets) {

	std::array<vk::DescriptorSetLayout, FRAMES_IN_FLIGHT> layouts = { layout, layout };

	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = engine->descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	engine->devContext.device.allocateDescriptorSets(&allocInfo, sets.data());
}

void DescriptorManager::buildDescriptorSets() {

	for (auto& [set, layout] : builderLayouts) {
		buidDBDescriptorSet(layout, builderDescriptorSets[set]);
	}

	// one time write to every descriptor set
	for (auto& info : builderDescriptorSetsDetails)
	{
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			updateDescriptorSet(i, info);
		}
	}
}

void DescriptorManager::updateDescriptorSet(int frame, descriptorSetInfo& info) {
	vk::WriteDescriptorSet descriptorWrite{};

	descriptorWrite.dstSet = builderDescriptorSets[info.set][frame];
	descriptorWrite.dstBinding = info.binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = info.type;

	if (info.type == vk::DescriptorType::eUniformBuffer || info.type == vk::DescriptorType::eStorageBuffer) {
		assert(info.doubleBuffer != nullptr);
		assert(info.bufferRange != 0);

		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = (*info.doubleBuffer)[frame];
		bufferInfo.offset = 0;
		bufferInfo.range = info.bufferRange;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		engine->devContext.device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
	}
	else if (info.type == vk::DescriptorType::eCombinedImageSampler) {

		assert(info.textures != nullptr);

		vector<vk::DescriptorImageInfo> imageInfos(info.textureCount);
		for (size_t i = 0; i < info.textureCount; i++) {
			vk::DescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.imageView = info.textures[i].imageView;
			imageInfo.sampler = info.textures[i].sampler;
			imageInfos[i] = imageInfo;
		}

		descriptorWrite.descriptorCount = info.textureCount;
		descriptorWrite.pImageInfo = imageInfos.data();

		engine->devContext.device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
	}
	else {
		assert(false);
	}
}