#include "stdafx.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <stb_image.h>

#include "VKengine.h"
#include "typedefs.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"n
#include "Vertex.h"
#include "GraphicsTemplate.h"

#include "ColoredTrianglesPL.h"

using namespace glm;
using namespace std;

void ColoredTrianglesPL::CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, MappedDoubleBuffer<cameraUBO_s>& cameradb, bool flipFaces) {

	engine->createMappedBuffer(sizeof(Vertex) * verticesPerMesh * ColoredTrianglesPL_MAX_OBJECTS, vk::BufferUsageFlagBits::eVertexBuffer, vertexDB);
	engine->createMappedBuffer(sizeof(InstanceBufferData) * ColoredTrianglesPL_MAX_OBJECTS, vk::BufferUsageFlagBits::eStorageBuffer, instanceDB);

	ShaderResourceConfig con;
	con.vertexSrc = vertexSrc;
	con.fragmentSrc = fragmentSrc;
	con.flipFaces = flipFaces;
	con.transparentFramebuffer = false;
	con.renderTarget = renderTarget;

	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, &cameradb.buffers, cameradb.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, &instanceDB.buffers, instanceDB.size));

	pipeline.CreateGraphicsPipeline(con);
}


void ColoredTrianglesPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int vertexCount) {

	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "colored triangles render");

	pipeline.bindPipelineResources(commandBuffer);

	vk::Buffer vertexBuffers[] = { vertexDB.buffers[engine->currentFrame] };
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

	vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}