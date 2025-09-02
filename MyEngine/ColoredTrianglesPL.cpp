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

#include "ColoredTrianglesPL.h"

using namespace glm;
using namespace std;

void ColoredTrianglesPL::CreateGraphicsPipeline(PipelineParameters& params) {

	engine->createMappedBuffer(sizeof(Vertex2D) * verticesPerMesh * maxTriangles, vk::BufferUsageFlagBits::eVertexBuffer, vertexDB);
	ShaderUtil::CreateMappedInstanceBuffer(engine, maxTriangles, instanceDB);

	PipelineResourceConfig con;
	con.bufferBindings.push_back(BufferBinding( 0, 0, params.cameraDB ));
	con.bufferBindings.push_back(BufferBinding( 0, 1, instanceDB ));

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