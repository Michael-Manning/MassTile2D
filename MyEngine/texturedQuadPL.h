#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <string>
#include <utility>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"

constexpr int TexturedQuadPL_MAX_TEXTURES = 10;
constexpr int TexturedQuadPL_MAX_OBJECTS = 100000;


class TexturedQuadPL :public  Pipeline {
public:

	struct ssboObjectInstanceData {

		alignas(8) glm::vec2 uvMin;
		alignas(8) glm::vec2 uvMax;

		alignas(8) glm::vec2 translation;
		alignas(8) glm::vec2 scale;
		float rotation = 0.0f;

		texID index;

		int32_t padding[2];
	};
	static_assert(sizeof(ssboObjectInstanceData) % 16 == 0);

	TexturedQuadPL(std::shared_ptr<VKEngine>& engine, VertexMeshBuffer quadMesh) : Pipeline(engine), quadMesh(quadMesh) {
		descriptorDirtyFlags.resize(FRAMES_IN_FLIGHT);
		bindIndexes.resize(FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			descriptorDirtyFlags[i] = true;
		}

		textures.resize(TexturedQuadPL_MAX_TEXTURES);
		for (size_t i = 0; i < TexturedQuadPL_MAX_TEXTURES; i++)
		{
			textures[i] = std::pair<texID, Texture*>(-1, { nullptr });
		}
	}

	void setDefaultTexture(Texture defaultTexture) {
		this->defaultTexture = defaultTexture;
	}

	void CreateGraphicsPipeline(std::string vertexSrc, std::string fragmentSrc);
	void createDescriptorSetLayout();
	void createDescriptorSets(MappedDoubleBuffer& cameradb) ;
	void createSSBOBuffer();

	void updateDescriptorSets();

	void addTextureBinding(texID ID, Texture* texture);
	void removeTextureBinding(texID ID);

	// mainly to force descriptor updates when filter mode changes
	void invalidateTextureDescriptors() {
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			descriptorDirtyFlags[i] = true;
		}
	};

	void recordCommandBuffer(VkCommandBuffer commandBuffer, std::vector<ssboObjectInstanceData>& drawlist);

	void UploadInstanceData(std::vector<ssboObjectInstanceData>& drawlist);
	void recordCommandBufferIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t stride);
	void GetDrawCommand(VkDrawIndexedIndirectCommand* cmd, int instanceCount);

private:

	VertexMeshBuffer quadMesh;

	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout SSBOSetLayout;

	MappedDoubleBuffer ssboMappedDB;

	std::vector<std::pair<texID, Texture*>> textures;
	std::set<texID> boundIDs;
	int bindingCount = 0;

	std::vector<std::unordered_map<texID, int>> bindIndexes; // texID to shader array index

	std::vector<bool> descriptorDirtyFlags;

	Texture defaultTexture; // display when indexing an unbound descriptor
};
