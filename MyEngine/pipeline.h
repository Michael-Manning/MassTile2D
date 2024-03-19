#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "texture.h"
#include "VKEngine.h"
#include "descriptorManager.h"
#include "GlobalImageDescriptor.h"
#include "globalBufferDefinitions.h"


struct PushConstantInfo {
	uint32_t pushConstantSize = 0;
	vk::ShaderStageFlags pushConstantShaderStages = static_cast<vk::ShaderStageFlags>(0);
};

struct BufferBinding {
	const int set;
	const int binding;
	const std::array<vk::Buffer, FRAMES_IN_FLIGHT>* buffers;
	const vk::DeviceSize size;
	const vk::BufferUsageFlags usage;

	template<typename T>
	BufferBinding(int set, int binding, const MappedDoubleBuffer<T>& mappedBuffer)
		: set(set), binding(binding), buffers(&mappedBuffer.buffers), size(mappedBuffer.size), usage(mappedBuffer.usage)
	{}

	BufferBinding(int set, int binding, const DeviceBuffer& buffer)
		: set(set), binding(binding), buffers(&buffer.doubleBuffer), size(buffer.size), usage(buffer.usage)
	{}
};

struct ShaderConstantBinding {
	const int constantID;
	size_t size;
};

struct PipelineParameters {
	std::vector<uint8_t> vertexSrc;
	std::vector<uint8_t> fragmentSrc;
	std::vector<std::vector<uint8_t>> computeSrcStages;
	vk::RenderPass renderTarget;
	MappedDoubleBuffer<coordinateTransformUBO_s> cameraDB;
	bool flipFaces = false;
};

struct PipelineResourceConfig {
	std::vector<DescriptorManager::descriptorSetInfo> descriptorInfos;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	std::vector<BufferBinding> bufferBindings;

	// populate to overide default
	std::optional< vk::PipelineColorBlendAttachmentState> colorBlendAttachment;
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
	vk::PipelineColorBlendStateCreateInfo defaultColorBlending(vk::PipelineColorBlendAttachmentState* attachment);
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
