#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "descriptorManager.h"
#include "GlobalImageDescriptor.h"
#include "pipeline.h"

class GraphicsTemplate : public Pipeline {
public:

	GraphicsTemplate(VKEngine* engine) : Pipeline(engine), descriptorManager(engine)
	{};

	void CreateGraphicsPipeline(const PipelineParameters& params, PipelineResourceConfig& resourceConfig);

	void bindPipelineResources(vk::CommandBuffer& commandBuffer);

	template<typename T>
	void UpdatePushConstant(vk::CommandBuffer& commandBuffer, T* pushConstantData) {

		static_assert(!std::is_void<T>::value, "T cannot be void");

		assert(pushInfo.pushConstantSize > 0);

		// only push size of struct as reflection may have evaluated the push constant size including padding.
		uint32_t size = sizeof(T);

		assert(size > 0 && size <= pushInfo.pushConstantSize);

		commandBuffer.pushConstants(pipelineLayout, pushInfo.pushConstantShaderStages, 0, size, pushConstantData);
	}


private:
	vk::Pipeline compPipeline;
	DescriptorManager descriptorManager;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	PushConstantInfo pushInfo;
	bool init = false;
};