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
#include "ParticleSystemPL.h"


void CreateGraphicsPipeline(const std::vector<uint8_t>& vertexSrc, const std::vector<uint8_t>& fragmentSrc, vk::RenderPass& renderTarget, MappedDoubleBuffer<cameraUBO_s>& cameradb, bool flipFaces = false) {

	ShaderResourceConfig con;
	con.vertexSrc = vertexSrc;
	con.fragmentSrc = fragmentSrc;
	con.flipFaces = flipFaces;
	con.renderTarget = renderTarget;

	con.descriptorInfos.push_back()

}

void createSSBOBuffer();

void recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount);

void UploadInstanceData(std::vector<particleSystem_ssbo>& drawlist);