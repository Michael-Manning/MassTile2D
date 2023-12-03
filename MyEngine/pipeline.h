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


class Pipeline {
public:

	Pipeline(std::shared_ptr<VKEngine> engine) : engine(engine) {
	}

protected:

	// for each frame in flight
	std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT> generalDescriptorSets;


	std::shared_ptr<VKEngine> engine = nullptr;

	vk::Pipeline _pipeline;
	vk::PipelineLayout pipelineLayout;

	

	// for builder
	struct descriptorSetInfo {
		int set;
		int binding;
		vk::DescriptorType type;
		vk::ShaderStageFlags stageFlags;
		std::array<vk::Buffer, FRAMES_IN_FLIGHT> * doubleBuffer = nullptr;
		vk::DeviceSize bufferRange = 0;
		Texture * textures= nullptr;
		int textureCount = 0;


		descriptorSetInfo(int set, int binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, std::array<vk::Buffer, FRAMES_IN_FLIGHT>* db, vk::DeviceSize bufferRange)
			: set(set), binding(binding), type(type), stageFlags(stageFlags), doubleBuffer(db), bufferRange(bufferRange),  textures(nullptr) {
		};
		descriptorSetInfo(int set, int binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, Texture* texture, int textureCount)
			: set(set), binding(binding), type(type), stageFlags(stageFlags), doubleBuffer(nullptr), bufferRange(0), textures(texture), textureCount(textureCount) {
		};
	};
	
	// set number to layout
	std::unordered_map<int, vk::DescriptorSetLayout> builderLayouts;
	std::unordered_map<int, std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT>> builderDescriptorSets;
	
	std::vector<descriptorSetInfo> builderDescriptorSetsDetails;

	void configureDescriptorSets(std::vector< descriptorSetInfo> layoutDetails) {
		builderDescriptorSetsDetails = layoutDetails;
	};

	void buildDescriptorLayouts();
	void buildDescriptorSets();
	void updateDescriptorSet(int frame, descriptorSetInfo& info);

	//std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages(std::string vertexSrc, std::string fragmentSrc);
	std::vector<vk::PipelineShaderStageCreateInfo> createShaderStages(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc);
	vk::PipelineShaderStageCreateInfo createComputeShaderStage(const std::vector<uint8_t>& computeSrc);
	//vk::PipelineShaderStageCreateInfo createComputeShaderStage(std::string computeSrc);

	vk::PipelineInputAssemblyStateCreateInfo defaultInputAssembly();
	vk::PipelineViewportStateCreateInfo defaultViewportState();
	vk::PipelineRasterizationStateCreateInfo defaultRasterizer();
	vk::PipelineMultisampleStateCreateInfo defaultMultisampling();
	vk::PipelineColorBlendAttachmentState defaultColorBlendAttachment(bool blendEnabled);
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

	vk::DescriptorSetLayoutBinding buildSamplerBinding(int binding, int descriptorCount, vk::ShaderStageFlags stageFlags);
	vk::DescriptorSetLayoutBinding buildUBOBinding(int binding, vk::ShaderStageFlags stageFlags);
	vk::DescriptorSetLayoutBinding buildSSBOBinding(int binding, vk::ShaderStageFlags stageFlags);
	void buildSetLayout(std::vector<vk::DescriptorSetLayoutBinding>& bindings, vk::DescriptorSetLayout& layout);

	void buildPipelineLayout(std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts, uint32_t pushConstantSize = 0, vk::ShaderStageFlags pushConstantStages = vk::ShaderStageFlags{});

	void buidDBDescriptorSet(vk::DescriptorSetLayout& layout, std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT>& sets);
};
