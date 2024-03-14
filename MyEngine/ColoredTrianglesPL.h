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
#include <vk_mem_alloc.h>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "globalBufferDefinitions.h"

constexpr int ColoredTrianglesPL_MAX_OBJECTS = 10000;

class ColoredTrianglesPL {
public:

	struct InstanceBufferData {
		glm::vec4 color;
	};
	static_assert(sizeof(InstanceBufferData) % 16 == 0);

	ColoredTrianglesPL(VKEngine* engine) : pipeline(engine), engine(engine) {
	}

	// move this struct 
	struct GraphicsPipelineConfiguration {
		std::vector<uint8_t> vertexSrc;
		std::vector<uint8_t> fragmentSrc;
		vk::RenderPass& renderTarget;
		MappedDoubleBuffer<cameraUBO_s>& cameradb;
		bool flipFaces = false;

		// add specialization constant settings
		bool invertLocalvertical;
	//	asfapsoeifnapseoifnawpseoif
	};

	void CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, MappedDoubleBuffer<cameraUBO_s>& cameradb, bool flipFaces = false);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int vertexCount);
	Vertex* GetVertexMappedBuffer(int frame) {
		return vertexDB.buffersMapped[frame];
	}
	InstanceBufferData* GetColorMappedBuffer(int frame) {
		return instanceDB.buffersMapped[frame];
	}

	const static int verticesPerMesh = 3;
private:
	GraphicsTemplate pipeline;

	MappedDoubleBuffer<Vertex> vertexDB;
	MappedDoubleBuffer<InstanceBufferData> instanceDB;

	VKEngine* engine;

};
