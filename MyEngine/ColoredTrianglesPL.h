#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "globalBufferDefinitions.h"

class ColoredTrianglesPL {
public:

	ColoredTrianglesPL(VKEngine* engine, int maxTriangles) : pipeline(engine), engine(engine), maxTriangles(maxTriangles) {
	}

	void CreateGraphicsPipeline(PipelineParameters& params);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int triangleCount);

	Vertex2D* GetVertexMappedBuffer(int frame) {
		return vertexDB.buffersMapped[frame];
	}
	ShaderTypes::ColoredTriangleInstance* GetColorMappedBuffer(int frame) {
		return instanceDB.buffersMapped[frame]->instanceData;
	}

	const static int verticesPerMesh = 3;

	const int maxTriangles;
private:
	GraphicsTemplate pipeline;

	MappedDoubleBuffer<Vertex2D> vertexDB;
	MappedDoubleBuffer<ShaderTypes::ColoredTriangleInstaceBuffer> instanceDB;

	VKEngine* engine;
};
