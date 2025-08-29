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
#include "GraphicsTemplate.h"

#include "TilemapLightRasterPL.h"

using namespace glm;
using namespace std;


void TilemapLightRasterPL::CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, TileWorldDeviceResources* tileWorldData) {

	this->tileWorldData = tileWorldData;

	PipelineResourceConfig con;
	con.bufferBindings.push_back(BufferBinding(1, 0, params.cameraDB));
	con.bufferBindings.push_back(BufferBinding(1, 1, tileWorldData->MapFGBuffer));
	con.bufferBindings.push_back(BufferBinding(1, 2, tileWorldData->MapBGBuffer));
	con.bufferBindings.push_back(BufferBinding(1, 3, tileWorldData->MapLightUpscaleBuffer));
	con.bufferBindings.push_back(BufferBinding(1, 4, tileWorldData->MapLightBlurBuffer));

	con.globalDescriptors.push_back({ 0, textureDescriptor });

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