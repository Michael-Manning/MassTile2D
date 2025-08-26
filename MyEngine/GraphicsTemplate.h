#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "descriptorManager.h"
#include "GlobalImageDescriptor.h"
#include "pipeline.h"

class GraphicsTemplate {
public:

	GraphicsTemplate(VKEngine* engine)
		: layoutCtx(engine), descriptorManager(engine), engine(engine)
	{
	};

	void CreateGraphicsPipeline(const PipelineParameters& params, PipelineResourceConfig& resourceConfig, const vkctx::PipelineOverrides& pipelineOverrides);

	void bindPipelineResources(vk::CommandBuffer& commandBuffer);

	template<typename T>
	void UpdatePushConstant(vk::CommandBuffer& commandBuffer, T&& pushConstantData) {

		static_assert(!std::is_void<T>::value, "T cannot be void");

		assert(pushInfo.pushConstantSize > 0);

		// only push the size of the struct as the reflection may have evaluated the push constant size including padding.
		uint32_t size = sizeof(T);

		assert(size > 0 && size <= pushInfo.pushConstantSize);

		commandBuffer.pushConstants(layoutCtx.pipelineLayout, pushInfo.pushConstantShaderStages, 0, size, &pushConstantData);
	}

	void UpdatePushConstant(vk::CommandBuffer& commandBuffer, const void* data, uint32_t size) {
		assert(pushInfo.pushConstantSize > 0);
		assert(size > 0 && size <= pushInfo.pushConstantSize);
		commandBuffer.pushConstants(layoutCtx.pipelineLayout, pushInfo.pushConstantShaderStages, 0, size, data);
	}

private:
	VKEngine* engine;
	vk::Pipeline gfxPipeline;
	PipelineLayoutCtx layoutCtx;
	DescriptorManager descriptorManager;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	PushConstantInfo pushInfo;
	bool init = false;
};