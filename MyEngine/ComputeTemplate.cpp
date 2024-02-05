#include "stdafx.h"

#include <memory>
#include <stdint.h>
#include <vector>

#include <glm/glm.hpp>

#include "descriptorManager.h"
#include "ComputeTemplate.h"

using namespace glm;
using namespace std;

void ComputeTemplate::CreateComputePipeline(const ShaderResourceConfig& resourceConfig) {
	auto computeStage = createComputeShaderStage(resourceConfig.computeSrc);

	this->pushInfo = resourceConfig.pushInfo;

	descriptorManager.configureDescriptorSets(resourceConfig.descriptorInfos);
	descriptorManager.buildDescriptorLayouts();


	descriptorLayoutMap setLayouts;

	for (auto& [set, layout] : descriptorManager.builderLayouts)
		setLayouts[set] = layout;

	for (auto& globalDesc : resourceConfig.globalDescriptors)
		setLayouts[globalDesc.setNumber] = globalDesc.descriptor->layout;


	assert(false);
	//buildPipelineLayout(setLayouts, pushInfo.pushConstantSize, pushInfo.pushConstantShaderStages);

	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.layout = pipelineLayout;

	{
		pipelineInfo.stage = computeStage;
		auto ret = engine->devContext.device.createComputePipeline({}, pipelineInfo);
		assert(ret.result == vk::Result::eSuccess);
		compPipeline = ret.value;
	}

	descriptorManager.buildDescriptorSets();

	// need to save for binding in command buffer
	globalDescriptors = resourceConfig.globalDescriptors;
}

void ComputeTemplate::recordCommandBuffer(vk::CommandBuffer& commandBuffer, glm::ivec3 workInstanceCounts, glm::ivec3 workGroupSizes, void* pushConstantData) {

	bindPipelineResources(commandBuffer, pushConstantData);

	Dispatch(commandBuffer, workInstanceCounts, workGroupSizes);
}

void ComputeTemplate::bindPipelineResources(vk::CommandBuffer& commandBuffer, void* pushConstantData) {
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compPipeline);

	// handle potential global descriptor
	for (auto& desc : globalDescriptors)
	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, desc.setNumber, 1, &desc.descriptor->descriptorSets[engine->currentFrame], 0, VK_NULL_HANDLE);
	}

	for (auto& detail : descriptorManager.builderDescriptorSetsDetails) {
		commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute,
			pipelineLayout,
			detail.set,
			1,
			reinterpret_cast<vk::DescriptorSet*>(&descriptorManager.builderDescriptorSets[detail.set][engine->currentFrame]),
			0,
			nullptr);
	}

	if (pushInfo.pushConstantSize > 0 && pushConstantData != nullptr) {
		updatePushConstant(commandBuffer, pushConstantData);
	}
}

void ComputeTemplate::updatePushConstant(vk::CommandBuffer& commandBuffer, void* pushConstantData) {
	commandBuffer.pushConstants(pipelineLayout, pushInfo.pushConstantShaderStages, 0, pushInfo.pushConstantSize, pushConstantData);
}

void ComputeTemplate::Dispatch(vk::CommandBuffer& commandBuffer, glm::ivec3 workInstanceCounts, glm::ivec3 workGroupSizes) {
	commandBuffer.dispatch(
		workInstanceCounts.x / workGroupSizes.x + (workInstanceCounts.x % workGroupSizes.x ? 1 : 0),
		workInstanceCounts.y / workGroupSizes.y + (workInstanceCounts.y % workGroupSizes.y ? 1 : 0),
		workInstanceCounts.z / workGroupSizes.z + (workInstanceCounts.z % workGroupSizes.z ? 1 : 0)
	);
}