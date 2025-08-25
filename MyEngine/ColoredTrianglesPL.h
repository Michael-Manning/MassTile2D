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

	struct InstanceBufferData {
		glm::vec4 color;
	};
	static_assert(sizeof(InstanceBufferData) % 16 == 0);

	ColoredTrianglesPL(VKEngine* engine, int maxTriangles) : pipeline(engine), engine(engine), maxTriangles(maxTriangles) {
	}

	void CreateGraphicsPipeline(PipelineParameters& params);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int triangleCount);

	Vertex2D* GetVertexMappedBuffer(int frame) {
		return vertexDB.buffersMapped[frame];
	}
	InstanceBufferData* GetColorMappedBuffer(int frame) {
		return instanceDB.buffersMapped[frame];
	}

	const static int verticesPerMesh = 3;

	const int maxTriangles;
private:
	GraphicsTemplate pipeline;

	MappedDoubleBuffer<Vertex2D> vertexDB;
	MappedDoubleBuffer<InstanceBufferData> instanceDB;

	VKEngine* engine;
};
