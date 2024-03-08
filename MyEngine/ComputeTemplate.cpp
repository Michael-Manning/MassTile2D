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

	assert(resourceConfig.computeSrcStages.size() > 0);

	auto computeStages = createComputeShaderStages(resourceConfig.computeSrcStages);

	this->pushInfo = resourceConfig.pushInfo;

	descriptorManager.configureDescriptorSets(resourceConfig.descriptorInfos);
	descriptorManager.buildDescriptorLayouts();


	descriptorLayoutMap setLayouts;

	for (auto& [set, layout] : descriptorManager.builderLayouts)
		setLayouts[set] = layout;

	for (auto& globalDesc : resourceConfig.globalDescriptors)
		setLayouts[globalDesc.setNumber] = globalDesc.descriptor->layout;


	buildPipelineLayout(setLayouts, pushInfo.pushConstantSize, pushInfo.pushConstantShaderStages);

	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.layout = pipelineLayout;

	compPipelines.reserve(computeStages.size());
	for (auto& stage : computeStages)
	{
		pipelineInfo.stage = stage;
		auto ret = engine->devContext.device.createComputePipeline({}, pipelineInfo);
		assert(ret.result == vk::Result::eSuccess);
		compPipelines.push_back(ret.value);
	}

	descriptorManager.buildDescriptorSets();

	// need to save for binding in command buffer
	globalDescriptors = resourceConfig.globalDescriptors;
}


void ComputeTemplate::BindDescriptorSets(vk::CommandBuffer& commandBuffer) {	

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
}

void ComputeTemplate::UpdatePushConstant(vk::CommandBuffer& commandBuffer, void* pushConstantData) {
	commandBuffer.pushConstants(pipelineLayout, pushInfo.pushConstantShaderStages, 0, pushInfo.pushConstantSize, pushConstantData);
}

/*

	Remember, if the layout of a shader is something like (32, 32, 1)
	and the dispatch call is (10, 1, 1)

	that will create 10 work groups have 1024 threads each which are layed out as 32 by 32.

	The axis in the dispatch call have no relation to the local size of the shader, it's just a multiplication of the two

*/

void ComputeTemplate::Dispatch(vk::CommandBuffer& commandBuffer, glm::ivec3 workInstancePerAxis, glm::ivec3 workGroupLocalSize) {

	int localThreadCount = workGroupLocalSize.x * workGroupLocalSize.y * workGroupLocalSize.z;

	commandBuffer.dispatch(
		workInstancePerAxis.x / localThreadCount + (workInstancePerAxis.x % localThreadCount ? 1 : 0),
		workInstancePerAxis.y / localThreadCount + (workInstancePerAxis.y % localThreadCount ? 1 : 0),
		workInstancePerAxis.z / localThreadCount + (workInstancePerAxis.z % localThreadCount ? 1 : 0)
	);
}

void ComputeTemplate::BindPipelineStage(vk::CommandBuffer& commandBuffer, int index)
{
	assert(index >= 0 && index < compPipelines.size());
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compPipelines[index]);
}
