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
	void CreateComputePipeline(const PipelineParameters& params, const PipelineResourceConfig& resourceConfig);

	void BindDescriptorSets(vk::CommandBuffer& commandBuffer);

	void UpdatePushConstant(vk::CommandBuffer& commandBuffer, void* pushConstantData);

	// dispatch a grid of threads. Will create the minimum number of groups in each axis to cover the grid size
	void DispatchGrid(vk::CommandBuffer& commandBuffer, glm::ivec3 gridSize, glm::ivec3 local_size);

	// wraps standard dispatch command
	void DispatchGroups(vk::CommandBuffer& commandBuffer, glm::ivec3 workGroupCounts);

	void BindPipelineStage(vk::CommandBuffer& commandBuffer, int index);

private:
	std::vector<vk::Pipeline> compPipelines;
	DescriptorManager descriptorManager;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	PushConstantInfo pushInfo;
};