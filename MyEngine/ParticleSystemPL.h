#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <string>
#include <utility>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "BindingManager.h"
#include "GlobalImageDescriptor.h"
#include "globalBufferDefinitions.h"
#include "GraphicsTemplate.h"

constexpr int MAX_PARTICLES_MEDIUM = 1000;


class ParticleSystemPL {
public:

	struct particle {
		alignas(8) glm::vec2 position;
		alignas(8) glm::vec2 velocity;
		float life;
		int32_t padding[2];
	};
	static_assert(sizeof(particle) % 16 == 0);

	struct particleSystem_ssbo {

		int32_t particleCount;
		particle particles[MAX_PARTICLES_MEDIUM];

		int32_t padding[2];
	};
	static_assert(sizeof(particleSystem_ssbo) % 16 == 0);

	ParticleSystemPL(std::shared_ptr<VKEngine>& engine) :
		pipeline(engine) { }

	void CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, MappedDoubleBuffer<cameraUBO_s>& cameradb, bool flipFaces = false);

	void createSSBOBuffer();

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount);

	void UploadInstanceData(std::vector<particleSystem_ssbo>& drawlist);

private:

	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<> ssboMappedDB;

	GraphicsTemplate pipeline;
};
