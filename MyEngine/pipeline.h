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


constexpr const char* shader_entry_point_name = "main";

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


// Increase as needed. Just preallocating to keep the ShaderConstantBinding struct simple.
constexpr size_t Max_spec_constant_size = 16;

struct SpecConstantBinding {
	const int constantID;
	const size_t size;
	const uint8_t* GetBindingData() {
		return bindingData.data();
	}

	template<typename T>
	SpecConstantBinding(int constantID, T bindingData)
		: constantID(constantID), size(sizeof(T))
	{
		static_assert(sizeof(T) <= Max_spec_constant_size);
		std::memcpy(this->bindingData.data(), &bindingData, sizeof(T));
	}

private:
	std::array<uint8_t, Max_spec_constant_size> bindingData;
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
	std::vector<SpecConstantBinding> specConstBindings;

	// populate to overide default
	std::optional< vk::PipelineColorBlendAttachmentState> colorBlendAttachment;
};

// keyed by set number
using descriptorLayoutMap = std::unordered_map<int, vk::DescriptorSetLayout>;


//// could move this function somewhere else
//inline void SetViewport(vk::CommandBuffer commandBuffer, glm::vec2 pos, glm::vec2 size) {
//	vk::Viewport viewport{};
//	viewport.x = pos.x;
//	viewport.y = pos.y;
//	viewport.width = size.x;
//	viewport.height = size.y;
//	viewport.minDepth = 0.0f;
//	viewport.maxDepth = 1.0f;
//	commandBuffer.setViewport(0, 1, &viewport);
//
//	vk::Rect2D scissor{};
//	scissor.offset = vk::Offset2D(0, 0);
//	scissor.extent = vk::Extent2D(static_cast<uint32_t>(abs(size.x)), static_cast<uint32_t>(abs(size.y)));
//	commandBuffer.setScissor(0, 1, &scissor);
//};
//
//inline void BindVertexMesh(vk::CommandBuffer commandBuffer, const VertexMeshBuffer& mesh) {
//	vk::Buffer vertexBuffers[] = { mesh.vertexBuffer };
//	vk::DeviceSize offsets[] = { 0 };
//	commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
//	commandBuffer.bindIndexBuffer(mesh.indexBuffer, 0, vk::IndexType::eUint16);
//};




class PipelineLayoutCtx {
public:

	PipelineLayoutCtx(VKEngine* engine) : engine(engine), descriptorManager(engine) {
	}

	DescriptorManager descriptorManager;

	// for each frame in flight
	std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT> generalDescriptorSets;

	VKEngine* engine = nullptr;

	vk::Pipeline _pipeline;
	vk::PipelineLayout pipelineLayout;

	struct GraphicsShaderInfo { vk::PipelineShaderStageCreateInfo vertex; vk::PipelineShaderStageCreateInfo fragment; };
	GraphicsShaderInfo createGraphicsShaderStages(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc) const;
	std::vector<vk::PipelineShaderStageCreateInfo> createComputeShaderStages(const std::vector<std::vector<uint8_t>>& computeSrcs) const;

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
