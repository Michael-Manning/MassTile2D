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
	int systemSize; // small = 0, large  = 1;
};

void ParticleSystemPL::CreateGraphicsPipeline(const PipelineParameters& params, const DeviceBuffer& deviceParticleDataBuffer){
	
	engine->createMappedBuffer(sizeof(host_particle_ssbo), vk::BufferUsageFlagBits::eStorageBuffer, particleDB);


	PipelineResourceConfig con;

	con.bufferBindings.push_back(BufferBinding(0, 1, params.cameraDB));
	con.bufferBindings.push_back(BufferBinding(0, 0, particleDB));
	con.bufferBindings.push_back(BufferBinding(0, 2, deviceParticleDataBuffer));
	
	//auto deviceDB = deviceParticleDataBuffer.GetDoubleBuffer();
	//con.descriptorInfos.push_back(DescriptorManager::descriptorSetInfo(0, 2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, &deviceDB, deviceParticleDataBuffer.size));

	con.pushInfo = PushConstantInfo{
		.pushConstantSize = sizeof(pushConstant_s),
		.pushConstantShaderStages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
	};

	pipeline.CreateGraphicsPipeline(params, con);
}

void ParticleSystemPL::recordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes, std::vector<int>& systemSizes, std::vector<int>& systemParticleCounts) {

	TracyVkZone(engine->tracyGraphicsContexts[engine->currentFrame], commandBuffer, "particle system render");

	pipeline.bindPipelineResources(commandBuffer);

	assert(systemIndexes.size() == systemParticleCounts.size());

	for (size_t i = 0; i < systemIndexes.size(); i++)
	{
		pushConstant_s pc{ 
			.systemIndex = systemIndexes[i],
			.systemSize = systemSizes[i]
		};
		pipeline.UpdatePushConstant(commandBuffer, &pc);
		commandBuffer.drawIndexed(static_cast<int32_t>(QuadIndices.size()), systemParticleCounts[i], 0, 0, 0);
	}
}
