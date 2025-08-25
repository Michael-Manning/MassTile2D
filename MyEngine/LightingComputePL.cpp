#include "stdafx.h"

#include <vector>

#include <vulkan/vulkan.hpp>
#include <tracy/Tracy.hpp>
#include <glm/glm.hpp>

#include "VKengine.h"
#include "typedefs.h"
#include "globalBufferDefinitions.h"
#include "ComputeTemplate.h"

#include "LightingComputePL.h"

using namespace glm;
using namespace std;

void LightingComputePL::CreateComputePipeline(const std::vector<uint8_t>& computeSrc_firstPass, const std::vector<uint8_t>& computeSrc_secondPass, const std::vector<uint8_t>& computeSrc_thirdPass, TileWorldDeviceResources* tileWorldData) {

	this->tileWorldData = tileWorldData;

	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkBaseLightingUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, baseLightUpdateDB);
	engine->createMappedBuffer(sizeof(chunkLightingUpdateinfo) * maxChunkBaseLightingUpdatesPerFrame, vk::BufferUsageFlagBits::eStorageBuffer, blurLightUpdateDB);

	PipelineParameters params;
	params.computeSrcStages = { computeSrc_firstPass, computeSrc_secondPass, computeSrc_thirdPass };

	PipelineResourceConfig con;
	con.bufferBindings.reserve(6);
	con.bufferBindings.push_back({ BufferBinding(0, 0, baseLightUpdateDB) });
	con.bufferBindings.push_back({ BufferBinding(0, 1, blurLightUpdateDB) });
	con.bufferBindings.push_back({ BufferBinding(1, 0, tileWorldData->MapFGBuffer) });
	con.bufferBindings.push_back({ BufferBinding(1, 1, tileWorldData->MapBGBuffer) });
	con.bufferBindings.push_back({ BufferBinding(1, 2, tileWorldData->MapLightUpscaleBuffer) });
	con.bufferBindings.push_back({ BufferBinding(1, 3, tileWorldData->MapLightBlurBuffer) });

	pipelines.CreateComputePipeline(params, con);
}

void LightingComputePL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int baseUpdates, int blurUpdates, const TileWorldLightingSettings_pc& lightingSettings) {

	if (baseUpdates == 0 && blurUpdates == 0)
		return;

	ZoneScoped;

	assert(baseUpdates <= maxChunkBaseLightingUpdatesPerFrame && blurUpdates <= maxChunkBaseLightingUpdatesPerFrame);

	pipelines.BindDescriptorSets(commandBuffer);
	pipelines.UpdatePushConstant(commandBuffer, lightingSettings); // does this need to be pushed for every pipeline stage?

	if (baseUpdates != 0)
	{
		TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting compute");
		pipelines.BindPipelineStage(commandBuffer, 0);
		pipelines.DispatchGroups(commandBuffer, { baseUpdates, 1, 1 });

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
			TracyVkZone(engine->tracyComputeContexts[engine->currentFrame], commandBuffer, "Lighting up");
			pipelines.BindPipelineStage(commandBuffer, 1);
			pipelines.DispatchGroups(commandBuffer, { blurUpdates, 2, 2 });
		}

		if (lightingSettings.blurEnabled)
		{
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
			pipelines.BindPipelineStage(commandBuffer, 2);

			if (lightingSettings.upscaleEnabled)
				pipelines.DispatchGroups(commandBuffer, { blurUpdates, 2, 2 });
			else
				pipelines.DispatchGroups(commandBuffer, { blurUpdates, 1, 1 });
		}
	}
}