#pragma once

#include <memory>
#include <vector>
#include <array>

#include <vulkan/vulkan.hpp>

#include "VKEngine.h"
#include "typedefs.h"
#include "texture.h"

class GlobalImageDescriptor {
public:
	GlobalImageDescriptor(std::shared_ptr<VKEngine> engine) : engine(engine) {
	}

	void CreateLayout(uint32_t binding, uint32_t descriptorCount, vk::Sampler* immutableSamplers = nullptr) {
		this->binding = binding;
		this->descriptorCount = descriptorCount;

		vk::DescriptorBindingFlags bindless_flags = vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::eUpdateAfterBind;

		vk::DescriptorSetLayoutBinding vk_binding;
		vk_binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		vk_binding.descriptorCount = descriptorCount;
		vk_binding.binding = binding;

		vk_binding.stageFlags = vk::ShaderStageFlagBits::eAll;
		vk_binding.pImmutableSamplers = immutableSamplers;


		vk::DescriptorSetLayoutCreateInfo layout_info;
		layout_info.bindingCount = 1;
		layout_info.pBindings = &vk_binding;
		layout_info.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

		vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT extended_info;
		extended_info.bindingCount = 1;
		extended_info.pBindingFlags = &bindless_flags;

		layout_info.pNext = &extended_info;

		layout = engine->devContext.device.createDescriptorSetLayout(layout_info);
	}

	void CreateDescriptors() {
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		{

			vk::DescriptorSetAllocateInfo alloc_info;
			alloc_info.descriptorPool = engine->descriptorPool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &layout;

			vk::DescriptorSetVariableDescriptorCountAllocateInfoEXT count_info;
			uint32_t max_binding = descriptorCount - 1;
			count_info.descriptorSetCount = 1;
			count_info.pDescriptorCounts = &max_binding; // This number is the max allocatable count
			alloc_info.pNext = &count_info;

			descriptorSets[i] = engine->devContext.device.allocateDescriptorSets(alloc_info)[0];
		}
	}

	void AddDescriptors(std::vector<int>& indexes, std::vector<std::shared_ptr<Texture>>& textures, int frame, vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal) {

		assert(indexes.size() == textures.size());
		if (indexes.size() == 0)
			return;

		std::vector<vk::WriteDescriptorSet> descriptorWrites(indexes.size());
		std::vector<vk::DescriptorImageInfo> imageInfos(indexes.size());

		for (size_t i = 0; i < indexes.size(); i++) {

			vk::WriteDescriptorSet descriptorWrite{};
			descriptorWrite.dstSet = static_cast<VkDescriptorSet>(descriptorSets[frame]);
			descriptorWrite.dstBinding = binding;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			descriptorWrite.dstArrayElement = indexes[i];

			vk::DescriptorImageInfo imageInfo{};
			imageInfo.imageView = textures[i]->imageView;
			imageInfo.sampler = textures[i]->sampler;
			imageInfo.imageLayout = imageLayout;
			imageInfos[i] = imageInfo;

			descriptorWrite.pImageInfo = &imageInfos[i];
			descriptorWrites[i] = descriptorWrite;
		}
		engine->devContext.device.updateDescriptorSets(indexes.size(), descriptorWrites.data(), 0, nullptr);
	}

	uint32_t GetBinding() {
		return binding;
	}

	vk::DescriptorSetLayout layout;
	std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT> descriptorSets;

private:
	uint32_t binding = 0;
	uint32_t descriptorCount = 0;
	std::shared_ptr<VKEngine> engine = nullptr;
};