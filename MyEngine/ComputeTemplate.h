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

	ComputeTemplate(std::shared_ptr<VKEngine>& engine) : Pipeline(engine), descriptorManager(engine)
	{};

	void CreateComputePipeline(const ShaderResourceConfig& resourceConfig);

	void recordCommandBuffer(vk::CommandBuffer& commandBuffer, glm::ivec3 workInstanceCounts, glm::ivec3 workGroupSizes, void* pushConstantData = nullptr);

private:
	vk::Pipeline compPipeline;
	DescriptorManager descriptorManager;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	PushConstantInfo pushInfo;
};