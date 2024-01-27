
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <stdint.h>
#include <cassert>

#include <vulkan/vulkan.hpp>

#include "Texture.h"
#include "AssetManager.h"
#include "typedefs.h"
#include "ResourceManager.h"

texID ResourceManager::GenerateTexture(int w, int h, std::vector<uint8_t>& data, FilterMode filterMode, bool imGuiTexure) {

	Texture tex = rengine->genTexture(w, h, data, filterMode);
	texID id;
	id = TextureIDGenerator.GenerateID();

	if (imGuiTexure)
		tex.imTexture = ImGui_ImplVulkan_AddTexture(tex.sampler, tex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	setTextureResource(id, tex);
	changeFlags->_flagTexturesAdded();
	return id;
}

texID ResourceManager::LoadTexture(std::string imagePath, FilterMode filterMode, bool imGuiTexure) {

	std::string filename = std::filesystem::path(imagePath).filename().string();

	assert(TextureIDGenerator.ContainsHash(filename) == false);

	Texture tex = rengine->genTexture(imagePath, filterMode);
	texID id = TextureIDGenerator.GenerateID(filename);

	if (imGuiTexure) 
		tex.imTexture = ImGui_ImplVulkan_AddTexture(tex.sampler, tex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	setTextureResource(id, tex);
	changeFlags->_flagTexturesAdded();
	return id;
}

texID ResourceManager::LoadTexture(uint8_t * imageFileData, int dataLength, std::string fileName, FilterMode filterMode, bool imGuiTexure) {

	assert(TextureIDGenerator.ContainsHash(fileName) == false);

	Texture tex = rengine->genTexture(imageFileData, dataLength, filterMode);
	texID id = TextureIDGenerator.GenerateID(fileName);

	if (imGuiTexure)
		tex.imTexture = ImGui_ImplVulkan_AddTexture(tex.sampler, tex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	setTextureResource(id, tex);
	changeFlags->_flagTexturesAdded();
	return id;
}

void ResourceManager::UpdateTexture(texID id, FilterMode filterMode) {

	assert(HasTexture(id));

	auto tex = &textureResources[id];
	tex->sampler = (filterMode == FilterMode::Nearest) ? rengine->textureSampler_nearest : rengine->textureSampler_linear;
	changeFlags->_flagTextureFiltersChanged();
}

texID ResourceManager::LoadTextureAsync(std::string imagePath, FilterMode filterMode) {

	std::string filename = std::filesystem::path(imagePath).filename().string();
	assert(TextureIDGenerator.ContainsHash(filename) == false);

	texID id = TextureIDGenerator.GenerateID(filename);

	textureJob job;
	job.imagePath = imagePath;
	job.filtermode;
	job.id = id;
	asyncQueue.push(job);

	return id;
}

void ResourceManager::uploadThreadFunc(ConcurrentQueue<textureJob>& asyncQueue) {
	while (uploadCancel.load(std::memory_order::relaxed) == false) {
		textureJob job;
		if (asyncQueue.wait_and_pop(job)) {

			deletion_mtx.lock();
			if (slatedForDeletion(job.id)) {
				cancelDeletion(job.id);
				deletion_mtx.unlock();
				continue;
			}
			deletion_mtx.unlock();

			Texture tex = rengine->genTexture(job.imagePath, job.filtermode, uploadContext);

			deletion_mtx.lock();

			if (slatedForDeletion(job.id)) {
				rengine->freeTexture(tex);
				cancelDeletion(job.id);
				deletion_mtx.unlock();
				continue;
			}

			changeFlags->_flagTexturesAdded();
			setTextureResource(job.id, tex);

			deletion_mtx.unlock();
		}
	}
}

framebufferID ResourceManager::CreateFramebuffer(glm::ivec2 size, glm::vec4 clearColor) {
	framebufferID id = fbIDGenerator.GenerateID();
	DoubleFrameBufferContext dfb;
	
	// store the empty textures here first, then populate them after
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		texID id;
		id = TextureIDGenerator.GenerateID();
		dfb.textureIDs[i] = id; 
		setTextureResource(id, {});
		dfb.textures[i] = GetTexture(id);
	}
	
	rengine->CreateDoubleFrameBuffer(size, dfb, rengine->defaultThreadContext, clearColor);

	
	changeFlags->_flagTexturesAdded();

	framebufferRefMap.insert(std::pair<framebufferID, DoubleFrameBufferContext>(id, dfb));

	return id;
};

DoubleFrameBufferContext* ResourceManager::GetFramebuffer(framebufferID id) {
	assert(framebufferRefMap.contains(id));
	auto it = framebufferRefMap.find(id);
	if (it != framebufferRefMap.end())
		return &(it->second);
	return nullptr;
}

void ResourceManager::ResizeFramebuffer(framebufferID framebuffer, glm::ivec2 size) {
	
	auto fb = GetFramebuffer(framebuffer);
	for (size_t i = 0; i < fb->resizeDirtyFlags.size(); i++)
		fb->resizeDirtyFlags[i] = true;
	fb->targetSize = size;
};

void ResourceManager::HandleFramebufferRecreation() {

	// recreate dirty framebuffers
	for (auto& [id, framebufferContext] : framebufferRefMap) {
		if (framebufferContext.resizeDirtyFlags[rengine->currentFrame]) {
			rengine->RecreateFramebuffer(framebufferContext.targetSize, &framebufferContext, rengine->currentFrame, rengine->defaultThreadContext);
			framebufferContext.resizeDirtyFlags[rengine->currentFrame] = false;
			changeFlags->_flagTextureFiltersChanged();
		}
	}
}