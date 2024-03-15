#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "descriptorManager.h"
#include "GlobalImageDescriptor.h"
#include "pipeline.h"

class ComputeTemplate : public Pipeline {
public:

	ComputeTemplate(VKEngine* engine) : Pipeline(engine), descriptorManager(engine)
	{};

	// will create multiple pipelines which are assumed to share the same descriptor sets
	void CreateComputePipeline(const PipelineParameters& params, PipelineResourceConfig& resourceConfig);

	void BindDescriptorSets(vk::CommandBuffer& commandBuffer);

	// dispatch a grid of threads. Will create the minimum number of groups in each axis to cover the grid size
	void DispatchGrid(vk::CommandBuffer& commandBuffer, glm::ivec3 gridSize, glm::ivec3 local_size);

	// wraps standard dispatch command
	void DispatchGroups(vk::CommandBuffer& commandBuffer, glm::ivec3 workGroupCounts);

	void BindPipelineStage(vk::CommandBuffer& commandBuffer, int index);

	template<typename T>
	void UpdatePushConstant(vk::CommandBuffer& commandBuffer, T&& pushConstantData) {

		static_assert(!std::is_void<T>::value, "T cannot be void");

		assert(pushInfo.pushConstantSize > 0);

		// only push size of struct as reflection may have evaluated the push constant size including padding.
		uint32_t size = sizeof(T);

		assert(size > 0 && size <= pushInfo.pushConstantSize);

		commandBuffer.pushConstants(pipelineLayout, pushInfo.pushConstantShaderStages, 0, size, &pushConstantData);
	}

private:
	std::vector<vk::Pipeline> compPipelines;
	DescriptorManager descriptorManager;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	PushConstantInfo pushInfo;
	bool init = false;
};