#pragma once

#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <stdint.h>

#include<vulkan/vulkan.hpp>

#include "Texture.h"

const static std::array<std::string, 2> ResourceManager_supportedExtensions = { ".png", ".jpg" };
class ResourceManager {
public:

	ResourceManager(std::shared_ptr<VKEngine> engine) : rengine(engine) {
	}

	Texture * GetTexture(texID id) {
		return &textureResources[id];
	}


	texID GenerateTexture(int w, int h, std::vector<uint8_t>& data, FilterMode filterMode, bool imGuiTexure = true);
	texID LoadTexture(std::string imagePath, FilterMode filterMode, bool imGuiTexure = true);
	texID LoadTexture(uint8_t* imageFileData, int dataLength, std::string fileName, FilterMode filterMode, bool imGuiTexure);

	void UpdateTexture(texID id, FilterMode filterMode);

	void FreeTexture(texID id) {
		assert(textureResources.contains(id));
		rengine->freeTexture(textureResources[id]);
		textureResources.erase(id);
		TextureIDGenerator.Erase(id);
	};

	bool HasTexture(texID id) {
		// assuming generator set is faster than map lookup?
		return TextureIDGenerator.Contains(id);
	}

	bool GetFilterModesDirty() {
		return filterModesChanged;
	};
	void ClearFilterModesDirty() {
		filterModesChanged = false;
	};

private:
	std::unordered_map<texID, Texture> textureResources;
	IDGenerator<texID> TextureIDGenerator;
	std::shared_ptr<VKEngine> rengine;
	bool filterModesChanged;
};