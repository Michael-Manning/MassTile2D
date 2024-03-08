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
	void CreateComputePipeline(const ShaderResourceConfig& resourceConfig);

	void BindDescriptorSets(vk::CommandBuffer& commandBuffer);

	void UpdatePushConstant(vk::CommandBuffer& commandBuffer, void* pushConstantData);

	void Dispatch(vk::CommandBuffer& commandBuffer, glm::ivec3 workInstancePerAxis, glm::ivec3 workGroupLocalSize);

	void BindPipelineStage(vk::CommandBuffer& commandBuffer, int index);

private:
	std::vector<vk::Pipeline> compPipelines;
	DescriptorManager descriptorManager;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	PushConstantInfo pushInfo;
};