#include "stdafx.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <fstream>
#include <chrono>
#include <memory>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VKengine.h"
#include "typedefs.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"
#include "Vertex2D.h"
#include "ShaderTypes.h"
#include "ShaderUtility.h"
#include "GraphicsTemplate.h"

using namespace glm;
using namespace std;

void TilemapPL::CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, TileWorldDeviceResources* tileWorldData) {

	PipelineResourceConfig con = ShaderUtil::tilemap_CreateBufferBindings(
		tileWorldData->MapFGBuffer,
		tileWorldData->MapBGBuffer,
		params.cameraDB,
		textureDescriptor
	);

	pipeline.CreateGraphicsPipeline(params, con);
}

void TilemapPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex, int lightMapIndex) {
	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "Tilemap render");

	pipeline.bindPipelineResources(commandBuffer);

	pipeline.UpdatePushConstant(commandBuffer, ShaderTypes::TilemapLightingSourceInfo{
		.textureIndex = textureIndex,
		.lightingBufferIndex = lightMapIndex
		});

	commandBuffer.drawIndexed(QuadIndices.size(), 1, 0, 0, 0);
}


void TilemapLightRasterPL::CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, TileWorldDeviceResources* tileWorldData) {

	this->tileWorldData = tileWorldData;

	PipelineResourceConfig con = ShaderUtil::tilemapLightRaster_CreateBufferBindings(
		tileWorldData->MapFGBuffer,
		tileWorldData->MapBGBuffer,
		tileWorldData->MapLightUpscaleBuffer,
		tileWorldData->MapLightBlurBuffer,
		params.cameraDB,
		textureDescriptor
	);


	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eR;
	//colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eG;
	//colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eB;
	//colorBlendAttachment.colorWriteMask |= vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

	con.colorBlendAttachment = colorBlendAttachment;

	pipeline.CreateGraphicsPipeline(params, con);
}

void TilemapLightRasterPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex, const ShaderTypes::LightingSettings& lightingSettings) {
	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "Tilemap lighting raster");

	pipeline.bindPipelineResources(commandBuffer);
	pipeline.UpdatePushConstant(commandBuffer, lightingSettings);

	commandBuffer.drawIndexed(QuadIndices.size(), 1, 0, 0, 0);
}


void LightingComputePL::CreateComputePipeline(const std::vector<uint8_t>& computeSrc_firstPass, const std::vector<uint8_t>& computeSrc_secondPass, const std::vector<uint8_t>& computeSrc_thirdPass, TileWorldDeviceResources* tileWorldData) {

	this->tileWorldData = tileWorldData;

	ShaderUtil::CreateMappedInstanceBuffer(engine, maxChunkBaseLightingUpdatesPerFrame, baseLightUpdateDB);
	ShaderUtil::CreateMappedInstanceBuffer(engine, maxChunkBaseLightingUpdatesPerFrame, blurLightUpdateDB);

	PipelineResourceConfig con1 = ShaderUtil::lighting_CreateBufferBindings(
		tileWorldData->MapFGBuffer,
		baseLightUpdateDB,
		tileWorldData->MapBGBuffer,
		blurLightUpdateDB,
		tileWorldData->MapLightUpscaleBuffer
	);

	PipelineResourceConfig con2 = ShaderUtil::lightingUpscale_CreateBufferBindings(
		tileWorldData->MapFGBuffer,
		baseLightUpdateDB,
		blurLightUpdateDB,
		tileWorldData->MapBGBuffer,
		tileWorldData->MapLightUpscaleBuffer,
		tileWorldData->MapLightBlurBuffer
	);

	PipelineResourceConfig con3 = ShaderUtil::lightingBlur_CreateBufferBindings(
		tileWorldData->MapFGBuffer,
		baseLightUpdateDB,
		blurLightUpdateDB,
		tileWorldData->MapBGBuffer,
		tileWorldData->MapLightUpscaleBuffer,
		tileWorldData->MapLightBlurBuffer
	);

	{
		PipelineParameters params;
		params.computeSrc = computeSrc_firstPass;
		pipeline_pass1.CreateComputePipeline(params, con1);
	}
	{
		PipelineParameters params;
		params.computeSrc = computeSrc_secondPass;
		pipeline_pass2.CreateComputePipeline(params, con2);
	}
	{
		PipelineParameters params;
		params.computeSrc = computeSrc_thirdPass;
		pipeline_pass3.CreateComputePipeline(params, con3);
	}
}


void LightingComputePL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int baseUpdates, int blurUpdates, const ShaderTypes::LightingSettings& lightingSettings) {

	if (baseUpdates == 0 && blurUpdates == 0)
		return;

	ZoneScoped;

	assert(baseUpdates <= maxChunkBaseLightingUpdatesPerFrame && blurUpdates <= maxChunkBaseLightingUpdatesPerFrame);

	if (baseUpdates != 0)
	{
		pipeline_pass1.BindDescriptorSets(commandBuffer);
		pipeline_pass1.UpdatePushConstant(commandBuffer, lightingSettings);

		TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting compute");
		pipeline_pass1.BindPipelineStage(commandBuffer);
		pipeline_pass1.DispatchGroups(commandBuffer, { baseUpdates, 1, 1 });
	}

	// first stage writes light values to background layer which second stage reads from. Must synchronize accesss to this buffer.
	if (baseUpdates != 0 && blurUpdates != 0)
	{
		vk::BufferMemoryBarrier barrier;
		barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = tileWorldData->MapFGBuffer.buffer;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE; // Could be changed to a portion, but I don't know if there would be a benefit

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			static_cast<vk::DependencyFlags>(0),
			0, nullptr,
			1, &barrier,
			0, nullptr
		);
	}

	if (blurUpdates != 0)
	{
		if (lightingSettings.upscaleEnabled)
		{
			pipeline_pass2.BindDescriptorSets(commandBuffer);
			pipeline_pass2.UpdatePushConstant(commandBuffer, lightingSettings);

			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting up");
			pipeline_pass2.BindPipelineStage(commandBuffer);
			pipeline_pass2.DispatchGroups(commandBuffer, { blurUpdates, 2, 2 });
		}

		if (lightingSettings.blurEnabled)
		{
			pipeline_pass3.BindDescriptorSets(commandBuffer);
			pipeline_pass3.UpdatePushConstant(commandBuffer, lightingSettings);

			vk::BufferMemoryBarrier barrier;
			barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.buffer = tileWorldData->MapLightUpscaleBuffer.buffer;
			barrier.offset = 0;
			barrier.size = VK_WHOLE_SIZE; // Could be changed to a portion, but I don't know if there would be a benefit

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eComputeShader,
				vk::PipelineStageFlagBits::eComputeShader,
				static_cast<vk::DependencyFlags>(0),
				0, nullptr,
				1, &barrier,
				0, nullptr
			);

			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting blur");
			pipeline_pass3.BindPipelineStage(commandBuffer);

			if (lightingSettings.upscaleEnabled)
				pipeline_pass3.DispatchGroups(commandBuffer, { blurUpdates, 2, 2 });
			else
				pipeline_pass3.DispatchGroups(commandBuffer, { blurUpdates, 1, 1 });
		}
	}

}