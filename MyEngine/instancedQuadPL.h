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
constexpr int InstancedQuadPL_MAX_OBJECTS = 100000;


class InstancedQuadPL :public  Pipeline {
public:

	struct ssboObjectData {

		alignas(8) glm::vec2 uvMin;
		alignas(8) glm::vec2 uvMax;

		alignas(8) glm::vec2 translation;
		alignas(8) glm::vec2 scale;
		float rotation = 0.0f;

		texID index;

		int32_t padding[2];
	};
	static_assert(sizeof(ssboObjectData) % 16 == 0);

	InstancedQuadPL(std::shared_ptr<VKEngine>& engine) : Pipeline(engine) {
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

	void CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) override;
	void createDescriptorSetLayout() override;
	void createDescriptorSets(MappedDoubleBuffer& cameradb) ;
	void createVertices() override;
	void createUniformBuffers();
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

	void recordCommandBuffer(VkCommandBuffer commandBuffer, std::vector<ssboObjectData>& drawlist);

private:

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
