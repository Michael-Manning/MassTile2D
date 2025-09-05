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
#include "ShaderTypes.h"
#include "ShaderUtility.h"

#include "pipelines.h"

using namespace glm;
using namespace std;

// Colord quad

void ColoredQuadPL::CreateGraphicsPipeline(const PipelineParameters& params) {
	ShaderUtil::CreateMappedInstanceBuffer(engine, maxInstances, instanceDataDB);
	PipelineResourceConfig con = ShaderUtil::coloredQuad_CreateBufferBindings(instanceDataDB, params.cameraDB);
	pipeline.CreateGraphicsPipeline(params, con);
}

void ColoredQuadPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount) {
	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "Colored quad render");
	pipeline.bindPipelineResources(commandBuffer);
	commandBuffer.drawIndexed(QuadIndices.size(), instanceCount, 0, 0, 0);
}

//  Colored triangles

void ColoredTrianglesPL::CreateGraphicsPipeline(PipelineParameters& params) {

	engine->createMappedBuffer(sizeof(Vertex2D) * verticesPerMesh * maxTriangles, vk::BufferUsageFlagBits::eVertexBuffer, vertexDB);
	ShaderUtil::CreateMappedInstanceBuffer(engine, maxTriangles, instanceDB);
	PipelineResourceConfig con = ShaderUtil::coloredTriangles_CreateBufferBindings(instanceDB, params.cameraDB);

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

// Textured quad

void TexturedQuadPL::CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, bool lightMapEnabled, std::array<int, 2> lightMapTextureIndexes) {

	ShaderUtil::CreateMappedInstanceBuffer(engine, maxInstances, instanceDB);

	engine->createMappedBuffer(vk::BufferUsageFlagBits::eUniformBuffer, lightmapIndexDB);
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		lightmapIndexDB.buffersMapped[i]->lightMapIndex = lightMapTextureIndexes[i];

	PipelineResourceConfig con;
	con.bufferBindings.push_back(BufferBinding(1, 1, params.cameraDB));
	con.bufferBindings.push_back(BufferBinding(1, 0, instanceDB));
	con.bufferBindings.push_back(BufferBinding(1, 2, lightmapIndexDB));

	con.specConstBindings.push_back(SpecConstantBinding{ 0, static_cast<uint32_t>(lightMapEnabled ? 1 : 0) });

	con.globalDescriptors.push_back({ 0, textureDescriptor });

	pipeline.CreateGraphicsPipeline(params, con);
}

void TexturedQuadPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount) {
	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "textured quad render");

	assert(instanceCount > 0);

	pipeline.bindPipelineResources(commandBuffer);

	commandBuffer.drawIndexed(QuadIndices.size(), instanceCount, 0, 0, 0);
}