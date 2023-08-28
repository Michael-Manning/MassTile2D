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

constexpr int TexturedQuadPL_MAX_TEXTURES = 10;

class TexturedQuadPL :public  Pipeline {
public:

	struct DrawItem {
		texID texture;
		glm::vec2 position;
		glm::vec2 scale;
		float rotation;
	};


	TexturedQuadPL(std::shared_ptr<VKEngine>& engine) : Pipeline(engine) {
		descriptorDirtyFlags.resize(FRAMES_IN_FLIGHT);
		bindIndexes.resize(FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			descriptorDirtyFlags[i] = true;
		}

		textures.resize(TexturedQuadPL_MAX_TEXTURES);
		for (size_t i = 0; i < TexturedQuadPL_MAX_TEXTURES; i++)
		{
			textures[i] = std::pair<texID, Texture>(-1, { 0 });
		}
	}

	void setDefaultTexture(Texture defaultTexture) {
		this->defaultTexture = defaultTexture;
	}

	void CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) override;
	void createDescriptorSetLayout() override;
	void createDescriptorSets() override;
	void createVertices() override;
	void createUniformBuffers();

	void updateCamera(glm::vec2 pos, float zoom)
	{
		cameraPosition = pos;
		cameraZoom = zoom;
	}

	void updateUBO(float aspect);
	void updateDescriptorSets();

	void addTextureBinding(texID ID, Texture texture);
	void removeTextureBinding(texID ID);

	//void setDrawList(std::vector<DrawItem>& list)
	//{
	//	drawlist = list;
	//}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, std::vector<DrawItem>& drawlist);

private:

	std::array<bool, FRAMES_IN_FLIGHT> uboDirtyFlags = { true, true };

	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;

	glm::vec2 cameraPosition = glm::vec2(0.0f);
	float cameraZoom = 1.0f;

	//std::vector<DrawItem> drawlist;

	std::vector<std::pair<texID, Texture>> textures;
	std::set<texID> boundIDs;
	int bindingCount = 0;

	std::vector<std::unordered_map<texID, int>> bindIndexes;

	std::vector<bool> descriptorDirtyFlags;

	Texture defaultTexture; // display when indexing an unbound descriptor
};



class ColoredQuadPL :public  Pipeline {
public:

	struct DrawItem {
		glm::vec4 color;
		glm::vec2 position;
		glm::vec2 scale;
		int circle;
		float rotation;
	};


	ColoredQuadPL(std::shared_ptr<VKEngine>& engine) : Pipeline(engine) {
	}

	void CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) override;
	void createDescriptorSetLayout() override;
	void createDescriptorSets() override;
	void createVertices() override;
	void createUniformBuffers();

	void updateCamera(glm::vec2 pos, float zoom)
	{
		cameraPosition = pos;
		cameraZoom = zoom;
	}

	void updateUBO(float aspect);

	//void setDrawList(std::vector<DrawItem>& list)
	//{
	//	drawlist = list;
	//}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, std::vector<DrawItem>& drawlist);

	void invalidateAspectUBO() {
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
			uboDirtyFlags[i] = true;
	};

private:

	struct UBO_s {
		float aspectRatio = 1.0f;
	};
	UBO_s uboData;

	std::array<bool, FRAMES_IN_FLIGHT> uboDirtyFlags = { true, true };

	VkDescriptorSetLayout descriptorSetLayout;

	glm::vec2 cameraPosition = glm::vec2(0.0f);
	float cameraZoom = 1.0f;

	//std::vector<DrawItem> drawlist;
};


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
	void createDescriptorSets() override;
	void createVertices() override;
	void createUniformBuffers();
	void createSSBOBuffer();

	void updateCamera(glm::vec2 pos, float zoom)
	{
		cameraPosition = pos;
		cameraZoom = zoom;
	}

	void updateUBO(float aspect);
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

	void invalidateAspectUBO() {
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
			uboDirtyFlags[i] = true;
	};

private:

	struct UBO_s {
		float aspectRatio = 1.0f;
	};
	UBO_s uboData;

	std::array<bool, FRAMES_IN_FLIGHT> uboDirtyFlags = { true, true };

	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout SSBOSetLayout;

	MappedDoubleBuffer ssboMappedDB;

	glm::vec2 cameraPosition = glm::vec2(0.0f);
	float cameraZoom = 1.0f;


	std::vector<std::pair<texID, Texture*>> textures;
	std::set<texID> boundIDs;
	int bindingCount = 0;

	std::vector<std::unordered_map<texID, int>> bindIndexes; // texID to shader array index

	std::vector<bool> descriptorDirtyFlags;

	Texture defaultTexture; // display when indexing an unbound descriptor
};
//
//constexpr int TilemapPL_MAX_TILES = 1048576 * 8;
//constexpr int largeChunkCount = 131072;
//constexpr int mapW = 1024 * 4;
//constexpr int mapH = 1024 * 2;


constexpr int TilemapPL_MAX_TILES = 1048576 * 2;
constexpr int largeChunkCount = 131072;
constexpr int mapW = 1024 * 2;
constexpr int mapH = 1024 * 1;
constexpr int mapCount = mapW * mapH;

constexpr int chunkSize = 32;
constexpr int chunkTileCount = chunkSize * chunkSize;


constexpr int chunksX = mapW / chunkSize;
constexpr int chunksY = mapH / chunkSize;
constexpr int chunkCount = chunksX * chunksY;

static_assert(mapCount == TilemapPL_MAX_TILES);
static_assert(mapW% chunkSize == 0);
static_assert(mapH% chunkSize == 0);


class TilemapPL :public  Pipeline {
public:

	struct ssboObjectData {
		blockID index;
	};
	//static_assert(sizeof(ssboObjectData) % 16 == 0);

	TilemapPL(std::shared_ptr<VKEngine>& engine) : Pipeline(engine) {
		mapData.resize(TilemapPL_MAX_TILES);
		chunkDirtyFlags = std::vector<bool>(chunkCount, false);
	}


	void CreateGraphicsPipline(std::string vertexSrc, std::string fragmentSrc) override;
	void createDescriptorSetLayout() override;
	void createDescriptorSets() override;
	void createVertices() override;
	void createUniformBuffers();
	void createSSBOBuffer();
	void createWorldBuffer();
	void createChunkTransferBuffers();


	void updateCamera(Camera& camera)
	{
		this->camera = camera;
	};

	void updateUBO(float aspect);

	void recordCommandBuffer(VkCommandBuffer commandBuffer);

	void setTile(uint32_t x, uint32_t y, blockID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chunkIndexOffset = chunk * chunkTileCount;
		mapData[chunkIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize)] = block;
		chunkDirtyFlags[chunk] = true;
		minDirtyIndex = chunk < minDirtyIndex ? chunk : minDirtyIndex;
		maxDirtyIndex = chunk > maxDirtyIndex ? chunk : maxDirtyIndex;
	};

	void preloadTile(uint32_t x, uint32_t y, blockID block) {
		uint32_t cx = (x / chunkSize);
		uint32_t cy = (y / chunkSize);
		uint32_t chunk = cy * chunksX + cx;
		uint32_t chuckIndexOffset = chunk * chunkTileCount;
		mapData[chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize)] = block;
	};
	
	int minDirtyIndex = INT32_MAX;
	int maxDirtyIndex = -1;
	std::vector<bool> chunkDirtyFlags;
	std::vector<blockID> mapData;

	std::optional<Texture> textureAtlas;

	void copyToLargeChunkTransferbuffer(uint32_t* data) {
		memcpy(largeChunkBufferMapped, data, sizeof(ssboObjectData) * largeChunkCount);
	}

	void copyLargeChunkToDevice(int chunkIndex) {
		engine->copyBuffer(largeChunkBuffer, worldMapDeviceBuffer, sizeof(ssboObjectData) * largeChunkCount, chunkIndex * sizeof(ssboObjectData) * largeChunkCount);
	}

	void invalidateAspectUBO() {
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
			uboDirtyFlags[i] = true;
	};

	void stageChunkUpdates(VkCommandBuffer commandBuffer);

private:



	struct UBO_s {
		float aspectRatio = 1.0f;
	};
	UBO_s uboData;

	std::array<bool, FRAMES_IN_FLIGHT> uboDirtyFlags = { true, true };

	std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> ssboDescriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout SSBOSetLayout;

	//MappedDoubleBuffer ssboMappedDB;

	VkBuffer largeChunkBuffer;
	VmaAllocation largeChunkAllocation;
	void* largeChunkBufferMapped;

	MappedDoubleBuffer chunkTransferBuffers;

	VkBuffer worldMapDeviceBuffer;
	VmaAllocation worldMapDeviceBufferAllocation;

	Camera camera;
};
