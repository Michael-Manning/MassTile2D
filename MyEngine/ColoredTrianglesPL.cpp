#include "stdafx.h"

#include <vector>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "VKengine.h"
#include "typedefs.h"
#include "globalBufferDefinitions.h"
#include "GraphicsTemplate.h"

#include "ColoredTrianglesPL.h"

using namespace glm;
using namespace std;

void ColoredTrianglesPL::CreateGraphicsPipeline(PipelineParameters& params) {

	engine->createMappedBuffer(sizeof(Vertex) * verticesPerMesh * maxTriangles, vk::BufferUsageFlagBits::eVertexBuffer, vertexDB);
	engine->createMappedBuffer(sizeof(InstanceBufferData) * maxTriangles, vk::BufferUsageFlagBits::eStorageBuffer, instanceDB);

	PipelineResourceConfig con;
	con.bufferBindings.push_back(BufferBinding( 0, 0, params.cameraDB ));
	con.bufferBindings.push_back(BufferBinding( 0, 1, instanceDB ));

	/*con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, &params.cameraDB.buffers, params.cameraDB.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, &instanceDB.buffers, instanceDB.size));*/

	pipeline.CreateGraphicsPipeline(params, con);
}


void ColoredTrianglesPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int triangleCount) {

	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "colored triangles render");

	pipeline.bindPipelineResources(commandBuffer);

	vk::Buffer vertexBuffers[] = { vertexDB.buffers[engine->currentFrame] };
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

	vkCmdDraw(commandBuffer, triangleCount * verticesPerMesh, 1, 0, 0);
}