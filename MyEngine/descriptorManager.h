#pragma once
#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include "texture.h"
#include "VKEngine.h"

// handles layouts, allocation, and creation of descriptor sets for common use cases.
// Supports the following:
// - shader storage buffers
// - uniform buffers
// - single texture samplers
// - texture sampler arrays
class DescriptorManager {
public:

	DescriptorManager(VKEngine* engine) : engine(engine) {
	}

	struct descriptorSetInfo {
		int set;
		int binding;
		vk::DescriptorType type;
		vk::ShaderStageFlags stageFlags;
		const std::array<vk::Buffer, FRAMES_IN_FLIGHT>* doubleBuffer = nullptr;
		vk::DeviceSize bufferRange = 0;
		Texture* textures = nullptr;
		int textureCount = 0;

		descriptorSetInfo(int set, int binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, const std::array<vk::Buffer, FRAMES_IN_FLIGHT>* db, vk::DeviceSize bufferRange)
			: set(set), binding(binding), type(type), stageFlags(stageFlags), doubleBuffer(db), bufferRange(bufferRange), textures(nullptr) {
		};
		descriptorSetInfo(int set, int binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, Texture* texture, int textureCount)
			: set(set), binding(binding), type(type), stageFlags(stageFlags), doubleBuffer(nullptr), bufferRange(0), textures(texture), textureCount(textureCount) {
		};
	};

	std::vector<descriptorSetInfo> builderDescriptorSetsDetails;
	void configureDescriptorSets(std::vector<descriptorSetInfo> layoutDetails) {
		builderDescriptorSetsDetails = layoutDetails;
	};

	void buildDescriptorLayouts();
	void buildDescriptorSets();
	void updateDescriptorSet(int frame, descriptorSetInfo& info);

	// set number to layout
	std::unordered_map<int, vk::DescriptorSetLayout> builderLayouts;
	std::unordered_map<int, std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT>> builderDescriptorSets;

private:

	vk::DescriptorSetLayoutBinding buildSSBOBinding(int binding, vk::ShaderStageFlags stageFlags);
	vk::DescriptorSetLayoutBinding buildUBOBinding(int binding, vk::ShaderStageFlags stageFlags);
	vk::DescriptorSetLayoutBinding buildSamplerBinding(int binding, int descriptorCount, vk::ShaderStageFlags stageFlags);

	void buildSetLayout(std::vector<vk::DescriptorSetLayoutBinding>& bindings, vk::DescriptorSetLayout& layout);

	void buidDBDescriptorSet(vk::DescriptorSetLayout& layout, std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT>& sets);

	VKEngine* engine = nullptr;
};
