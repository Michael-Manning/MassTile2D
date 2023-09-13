#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <fstream>
#include <chrono>
#include <memory>
#include <utility>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <stb_image.h>

#include "VKengine.h"
#include "typedefs.h"
#include "vulkan_util.h"
#include "texturedQuadPL.h"
#include "globalBufferDefinitions.h"
#include "Vertex.h"

using namespace glm;
using namespace std;


void TexturedQuadPL::CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc, MappedDoubleBuffer<void>& cameradb) {

	assert(defaultTexture.textureImageAllocation != nullptr);

	auto shaderStages = createShaderStages(vertexSrc, fragmentSrc);

	array<Texture, TexturedQuadPL_MAX_TEXTURES> defaultTextureArray;
	std::fill(defaultTextureArray.begin(), defaultTextureArray.end(), defaultTexture);

	configureDescriptorSets(vector<Pipeline::descriptorSetInfo> {
		descriptorSetInfo(0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, &cameradb.buffers, cameradb.size),
		descriptorSetInfo(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, defaultTextureArray.data(), defaultTextureArray.size()),
		descriptorSetInfo(1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &ssboMappedDB.buffers, ssboMappedDB.size)
	});
	buildDescriptorLayouts();

	// create vector containing the builder descriptor set layouts
	vector< VkDescriptorSetLayout> setLayouts;
	setLayouts.reserve(builderLayouts.size());
	for (auto& [set, layout] : builderLayouts)
		setLayouts.push_back(layout);
	buildPipelineLayout(setLayouts);

	VkVertexInputBindingDescription VbindingDescription;
	dbVertexAtribute Vattribute;
	auto vertexInputInfo = Vertex::getVertexInputInfo(&VbindingDescription, &Vattribute);

	auto inputAssembly = defaultInputAssembly();
	auto viewportState = defaultViewportState();
	auto rasterizer = defaultRasterizer();
	auto multisampling = defaultMultisampling();
	auto colorBlendAttachment = defaultColorBlendAttachment(true);
	auto colorBlending = defaultColorBlending(&colorBlendAttachment);
	auto dynamicState = defaultDynamicState();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
	pipelineInfo.renderPass = engine->renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	auto res = vkCreateGraphicsPipelines(engine->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
	assert(res == VK_SUCCESS);

	for (auto& stage : shaderStages) {
		vkDestroyShaderModule(engine->device, stage.module, nullptr);
	}

	buildDescriptorSets();
}

// For now, update every descriptor array element. Could update individual elements as an optimization
void TexturedQuadPL::updateDescriptorSets() {

	if(bindingManager.IsDescriptorDirty(engine->currentFrame) == false)
		return;

	array<Texture, TexturedQuadPL_MAX_TEXTURES> textureArray;

	for (int i = 0; i < textureArray.size(); i++) {
		if (bindingManager.IsSlotInUse(i) == false)
			textureArray[i] = defaultTexture;
		else
			textureArray[i] = *bindingManager.getValueFromIndex(i);
	}

	auto info = descriptorSetInfo(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, textureArray.data(), textureArray.size());
	updateDescriptorSet(engine->currentFrame, info);

	bindingManager.ClearDescriptorDirty(engine->currentFrame);
}

void TexturedQuadPL::createSSBOBuffer() {

	engine->createMappedBuffer(sizeof(ssboObjectInstanceData) * TexturedQuadPL_MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, ssboMappedDB);
}

void TexturedQuadPL::recordCommandBuffer(VkCommandBuffer commandBuffer, int instanceCount){
	if (instanceCount == 0)
		return;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

	for (auto& i : builderDescriptorSetsDetails)
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, i.set, 1, &builderDescriptorSets[i.set][engine->currentFrame], 0, nullptr);

	{
		TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "textured quad render");
		vkCmdDrawIndexed(commandBuffer, QuadIndices.size(), instanceCount, 0, 0, 0);
	}
}

void TexturedQuadPL::UploadInstanceData(std::vector<ssboObjectInstanceData>& drawlist) {
	ZoneScopedN("texture quad data copy");
	if (drawlist.size() == 0)
		return;

	assert(drawlist.size() <= TexturedQuadPL_MAX_OBJECTS);

	for (auto& i : drawlist) {
		// replace texture with index in descriptor set
		i.tex = bindingManager.getIndexFromBinding(i.tex);
	}
	memcpy(ssboMappedDB.buffersMapped[engine->currentFrame], drawlist.data(), sizeof(ssboObjectInstanceData) * drawlist.size());
}