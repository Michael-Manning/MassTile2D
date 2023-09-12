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


void TexturedQuadPL::CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc) {

	auto shaderStages = createShaderStages(vertexSrc, fragmentSrc);

	// setup vertex
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	vector< VkDescriptorSetLayout> setLayouts = { descriptorSetLayout, SSBOSetLayout };
	buildPipelineLayout(setLayouts);

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
}

void TexturedQuadPL::createDescriptorSets(MappedDoubleBuffer& cameradb) {

	assert(defaultTexture.textureImageAllocation != nullptr);

	// general descriptor sets
	{
		buidDBDescriptorSet(descriptorSetLayout, generalDescriptorSets);

		// one time update from UBO descriptor sets
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = cameradb.buffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(cameraUBO_s);

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = generalDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		// all descriptor sets must recieve one update before use. Fill image descriptor with default texture
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			std::vector<VkDescriptorImageInfo> imageInfos(TexturedQuadPL_MAX_TEXTURES);

			for (size_t j = 0; j < TexturedQuadPL_MAX_TEXTURES; j++)
			{
				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = defaultTexture.imageView;
				imageInfo.sampler = defaultTexture.sampler;

				imageInfos[j] = imageInfo;
			}


			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = generalDescriptorSets[i];
			descriptorWrites[0].dstBinding = 1;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[0].descriptorCount = TexturedQuadPL_MAX_TEXTURES;
			descriptorWrites[0].pImageInfo = imageInfos.data();

			vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

	}

	// SSBO descriptor sets
	{
		buidDBDescriptorSet(SSBOSetLayout, ssboDescriptorSets);

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {

			VkDescriptorBufferInfo objectBufferInfo;
			//objectBufferInfo.buffer = ssboBuffers[i];
			objectBufferInfo.buffer = ssboMappedDB.buffers[i];
			objectBufferInfo.offset = 0;
			objectBufferInfo.range = sizeof(ssboObjectInstanceData) * TexturedQuadPL_MAX_OBJECTS;

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = ssboDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &objectBufferInfo;

			vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
}

// For now, update every descriptor array element. Could update individual elements as an optimization
void TexturedQuadPL::updateDescriptorSets() {

	if(bindingManager.IsDescriptorDirty(engine->currentFrame) == false)
		return;

	std::vector<VkDescriptorImageInfo> imageInfos(TexturedQuadPL_MAX_TEXTURES);

	for (int i = 0; i < TexturedQuadPL_MAX_TEXTURES; i++)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if(bindingManager.IsSlotInUse(i) == false){
			imageInfo.imageView = defaultTexture.imageView;
			imageInfo.sampler = defaultTexture.sampler;
		}
		else {
			auto t = bindingManager.getValueFromIndex(i);

			imageInfo.imageView = t->imageView;
			imageInfo.sampler = t->sampler;
		}

		imageInfos[i] = imageInfo;
	}

	std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = generalDescriptorSets[engine->currentFrame];
	descriptorWrites[0].dstBinding = 1;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[0].descriptorCount = TexturedQuadPL_MAX_TEXTURES;
	descriptorWrites[0].pImageInfo = imageInfos.data();

	vkUpdateDescriptorSets(engine->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	bindingManager.ClearDescriptorDirty(engine->currentFrame);
}


void TexturedQuadPL::createDescriptorSetLayout() {

	// general set layout
	vector<VkDescriptorSetLayoutBinding> set0Bindings = {
		buildUBOBinding(0, VK_SHADER_STAGE_VERTEX_BIT),
		buildSamplerBinding(1, TexturedQuadPL_MAX_TEXTURES, VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	buildSetLayout(set0Bindings, descriptorSetLayout);

	// ssbo set layout
	vector<VkDescriptorSetLayoutBinding> set1Bindings = {
		buildSSBOBinding(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	buildSetLayout(set1Bindings, SSBOSetLayout);
}

void TexturedQuadPL::createSSBOBuffer() {

	engine->createMappedBuffer(sizeof(ssboObjectInstanceData) * TexturedQuadPL_MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, ssboMappedDB);
}

void TexturedQuadPL::recordCommandBuffer(VkCommandBuffer commandBuffer, int instanceCount){
	if (instanceCount == 0)
		return;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &generalDescriptorSets[engine->currentFrame], 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &ssboDescriptorSets[engine->currentFrame], 0, nullptr);

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