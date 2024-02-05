#include "stdafx.h"

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
#include "Vertex.h"
#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "BindingManager.h"
#include "GlobalImageDescriptor.h"
#include "globalBufferDefinitions.h"
#include "ParticleSystemPL.h"

struct pushConstant_s {
	int systemIndex;
};

void ParticleSystemPL::CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, MappedDoubleBuffer<cameraUBO_s>& cameradb, bool flipFaces, bool transparentFramebuffer) {

	engine->createMappedBuffer(sizeof(host_particle_ssbo), vk::BufferUsageFlagBits::eStorageBuffer, particleDB);

	ShaderResourceConfig con;
	con.vertexSrc = vertexSrc;
	con.fragmentSrc = fragmentSrc;
	con.flipFaces = flipFaces;
	con.transparentFramebuffer = transparentFramebuffer;
	con.renderTarget = renderTarget;

	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, &cameradb.buffers, cameradb.size));
	con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, &particleDB.buffers, particleDB.size));

	con.pushInfo = PushConstantInfo{
		.pushConstantSize = sizeof(pushConstant_s),
		.pushConstantShaderStages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
	};

	pipeline.CreateGraphicsPipeline(con);
}

void ParticleSystemPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes, std::vector<int>& systemParticleCounts) {

	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "particle system render");


	pipeline.bindPipelineResources(commandBuffer);

	assert(systemIndexes.size() == systemParticleCounts.size());

	for (size_t i = 0; i < systemIndexes.size(); i++)
	{
		pushConstant_s pc{ .systemIndex = systemIndexes[i]};
		pipeline.updatePushConstant(commandBuffer, &pc);
		commandBuffer.drawIndexed(static_cast<int32_t>(QuadIndices.size()), systemParticleCounts[i], 0, 0, 0);
	}
}
