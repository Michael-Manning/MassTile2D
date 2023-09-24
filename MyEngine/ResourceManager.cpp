
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <stdint.h>
#include <cassert>

#include<vulkan/vulkan.hpp>

#include "Texture.h"
#include "AssetManager.h"

texID ResourceManager::GenerateTexture(int w, int h, std::vector<uint8_t>& data, FilterMode filterMode, bool imGuiTexure) {

	Texture tex = rengine->genTexture(w, h, data, filterMode);
	texID id;
	id = TextureIDGenerator.GenerateID();

	if (imGuiTexure)
		tex.imTexture = ImGui_ImplVulkan_AddTexture(tex.sampler, tex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	textureResources[id] = tex;
	return id;
}

texID ResourceManager::LoadTexture(std::string imagePath, FilterMode filterMode, bool imGuiTexure) {

	std::string filename = std::filesystem::path(imagePath).filename().string();

	assert(TextureIDGenerator.ContainsHash(filename) == false);

	Texture tex = rengine->genTexture(imagePath, filterMode);
	texID id = TextureIDGenerator.GenerateID(filename);

	if (imGuiTexure) 
		tex.imTexture = ImGui_ImplVulkan_AddTexture(tex.sampler, tex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	textureResources[id] = tex;
	return id;
}

void ResourceManager::UpdateTexture(texID id, FilterMode filterMode) {
	auto& tex = textureResources[id];
	tex.sampler = (filterMode == FilterMode::Nearest) ? rengine->textureSampler_nearest : rengine->textureSampler_linear;
	filterModesChanged = true;
}
