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

	GraphicsTemplate(std::shared_ptr<VKEngine>& engine) : Pipeline(engine), descriptorManager(engine)
	{};

	void CreateGraphicsPipeline(const ShaderResourceConfig& resourceConfig);

	void bindPipelineResources(vk::CommandBuffer& commandBuffer, void* pushConstantData = nullptr);

private:
	vk::Pipeline compPipeline;
	DescriptorManager descriptorManager;
	std::vector<GlobalDescriptorBinding> globalDescriptors;
	PushConstantInfo pushInfo;
};