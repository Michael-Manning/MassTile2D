#include "stdafx.h"

#include <memory>
#include <stdint.h>
#include <vector>

#include <glm/glm.hpp>

#include "descriptorManager.h"
#include "GraphicsTemplate.h"
#include "Reflection.h"
#include "pipeline.h"

using namespace glm;
using namespace std;

void GraphicsTemplate::CreateGraphicsPipeline(const PipelineParameters& params, PipelineResourceConfig& resourceConfig) {

	assert(init == false);
	init = true;

	auto shaderStages = createGraphicsShaderStages(params.vertexSrc, params.fragmentSrc);

	// binding info provided using buffer bindings means that we must use reflection to infer the shader stages
	{

		std::vector<Reflection::buffer_info> vertBufferInfos;
		std::vector<Reflection::buffer_info> fragBufferInfos;
		Reflection::push_constant_info vertPushInfo;
		Reflection::push_constant_info fragPushInfo;
		std::vector<Reflection::spec_constant_info> vertSpecInfos;
		std::vector<Reflection::spec_constant_info> fragSpecInfos;

		Reflection::GetShaderBufferBindings(params.vertexSrc, vertBufferInfos, vertPushInfo, vertSpecInfos);
		Reflection::GetShaderBufferBindings(params.fragmentSrc, fragBufferInfos, fragPushInfo, fragSpecInfos);

		if (vertPushInfo.size != 0 && fragPushInfo.size != 0) {
			assert(vertPushInfo.size == fragPushInfo.size);
		}

		if (vertPushInfo.size > 0) {
			pushInfo.pushConstantSize = vertPushInfo.size;
			pushInfo.pushConstantShaderStages |= vk::ShaderStageFlagBits::eVertex;
		}
		if (fragPushInfo.size > 0) {
			pushInfo.pushConstantSize = fragPushInfo.size;
			pushInfo.pushConstantShaderStages |= vk::ShaderStageFlagBits::eFragment;
		}

		if (resourceConfig.bufferBindings.size() > 0) {
			vector<vk::ShaderStageFlags> bindingShaderStages(resourceConfig.bufferBindings.size(), static_cast<vk::ShaderStageFlags>(0));

			int i = 0;
			for (auto& binding : resourceConfig.bufferBindings)
			{
				for (auto& info : vertBufferInfos)
					if (binding.set == info.set && binding.binding == info.binding)
						bindingShaderStages[i] |= vk::ShaderStageFlagBits::eVertex;
				for (auto& info : fragBufferInfos)
					if (binding.set == info.set && binding.binding == info.binding)
						bindingShaderStages[i] |= vk::ShaderStageFlagBits::eFragment;
				i++;
			}

			i = 0;
			for (; i < resourceConfig.bufferBindings.size(); i++) {
				const BufferBinding& binding = resourceConfig.bufferBindings[i];

				vk::DescriptorType type;
				if (binding.usage & vk::BufferUsageFlagBits::eUniformBuffer)
					type = vk::DescriptorType::eUniformBuffer;
				else if (binding.usage & vk::BufferUsageFlagBits::eStorageBuffer)
					type = vk::DescriptorType::eStorageBuffer;
				else {
					assert(false);
				}

				resourceConfig.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(binding.set, binding.binding, type, bindingShaderStages[i], binding.buffers, binding.size));
			}
		}
	}

	descriptorManager.configureDescriptorSets(resourceConfig.descriptorInfos);
	descriptorManager.buildDescriptorLayouts();


	descriptorLayoutMap setLayouts;

	for (auto& [set, layout] : descriptorManager.builderLayouts)
		setLayouts[set] = layout;

	for (auto& globalDesc : resourceConfig.globalDescriptors)
		setLayouts[globalDesc.setNumber] = globalDesc.descriptor->layout;


	buildPipelineLayout(setLayouts, pushInfo.pushConstantSize, pushInfo.pushConstantShaderStages);

	vk::VertexInputBindingDescription VbindingDescription;
	dbVertexAtribute Vattribute;
	auto vertexInputInfo = Vertex::getVertexInputInfo(&VbindingDescription, &Vattribute);

	auto inputAssembly = defaultInputAssembly();
	auto viewportState = defaultViewportState();
	auto rasterizer = defaultRasterizer();
	auto multisampling = defaultMultisampling();
	auto colorBlendAttachment = resourceConfig.colorBlendAttachment.has_value() ? resourceConfig.colorBlendAttachment.value() : defaultColorBlendAttachment(true, false);
	auto colorBlending = defaultColorBlending(&colorBlendAttachment);
	auto dynamicState = defaultDynamicState();

	if (params.flipFaces)
		rasterizer.frontFace = vk::FrontFace::eClockwise;

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = params.renderTarget;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	auto rv = engine->devContext.device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
	if (rv.result != vk::Result::eSuccess)
		throw std::runtime_error("failed to create graphics pipeline!");
	_pipeline = rv.value;

	for (auto& stage : shaderStages) {
		engine->devContext.device.destroyShaderModule(stage.module);
	}

	descriptorManager.buildDescriptorSets();

	// need to save for binding in command buffer
	globalDescriptors = resourceConfig.globalDescriptors;
}

void GraphicsTemplate::bindPipelineResources(vk::CommandBuffer& commandBuffer) {

	assert(init == true);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

	// handle potential global descriptor
	for (auto& desc : globalDescriptors)
	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, desc.setNumber, 1, &desc.descriptor->descriptorSets[engine->currentFrame], 0, VK_NULL_HANDLE);
	}

	for (auto& detail : descriptorManager.builderDescriptorSetsDetails) {
		commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			detail.set,
			1,
			reinterpret_cast<vk::DescriptorSet*>(&descriptorManager.builderDescriptorSets[detail.set][engine->currentFrame]),
			0,
			nullptr);
	}
}