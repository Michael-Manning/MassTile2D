#pragma once

#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <robin_hood.h>

#include<vulkan/vulkan.hpp>

#include "VKEngine.h"
#include "typedefs.h"
#include "Texture.h"
//#include "IDGenerator.h"
#include "SlotManager.h"
#include "ConcurrentQueue.h"

constexpr int MAX_TEXTURE_RESOURCES = 2000;

const static std::array<std::string, 2> ResourceManager_supportedExtensions = { ".png", ".jpg" };
class ResourceManager {
public:

	// thread-safe update flag manager
	class ChangeFlags {
	private:
		std::mutex mtx;
		bool texturesAdded = false;
		bool textureFiltersChanged = false;

	public:

		bool TexturesAdded() {
			std::unique_lock<std::mutex> lock(mtx);
			return texturesAdded;
		};
		bool TextureFiltersChanged() {
			std::unique_lock<std::mutex> lock(mtx);
			return textureFiltersChanged;
		};

		void ClearTexturesAdded() {
			std::unique_lock<std::mutex> lock(mtx);
			texturesAdded = false;
		};
		void ClearTextureFilterschanged() {
			std::unique_lock<std::mutex> lock(mtx);
			textureFiltersChanged = false;
		};

		void _flagTexturesAdded() {
			std::unique_lock<std::mutex> lock(mtx);
			texturesAdded = true;
		}
		void _flagTextureFiltersChanged() {
			std::unique_lock<std::mutex> lock(mtx);
			textureFiltersChanged = true;
		}

	};

	size_t UploadQueueLength() {
		return asyncQueue.size();
	}

	ResourceManager(VKEngine* engine, ChangeFlags* changeFlags)
		:
		rengine(engine),
		changeFlags(changeFlags),
		uploadContext(engine->GenerateThreadContext_gfx(true)),
		textureIDSlotManager(max_bindless_resources)
	{
		asyncThread = std::thread(&ResourceManager::uploadThreadFunc, this, std::ref(asyncQueue));
	};

	Texture * GetTexture(texID id) {
		std::unique_lock<std::mutex> lock(texMapMtx);
		if (textureResources.contains(id) == false)
			return nullptr;
		return &textureResources[id];
	}

	texID GetTextureID(std::string originalFilename) {
		assert(textureFileNameMap.contains(originalFilename));
		return textureFileNameMap.at(originalFilename);
	}
	
	const robin_hood::unordered_node_map<texID, Texture>* GetInternalTextureResources() {
		return &textureResources;
	}

	//auto _getTextureIterator() { return MapProxy<texID, std::shared_ptr<Texture>>(textureResources.begin(), textureResources.end()); };

	texID GenerateTexture(int w, int h, std::vector<uint8_t>& data, FilterMode filterMode, bool imGuiTexure = true);
	texID LoadTexture(std::string imagePath, FilterMode filterMode, bool imGuiTexure = true);
	texID LoadTexture(uint8_t* imageFileData, int dataLength, std::string fileName, FilterMode filterMode, bool imGuiTexure);

	texID LoadTextureAsync(std::string imagePath, FilterMode filterMode);

	void UpdateTexture(texID id, FilterMode filterMode);

	// will delete the texture once it is not in use
	void SetTextureFreeable(texID id);

	bool HasTexture(texID id) {
		std::unique_lock<std::mutex> lock(texMapMtx);
		return textureResources.contains(id);
	}
	bool HasTexture(std::string originalFilename) {
		std::unique_lock<std::mutex> lock(texMapMtx);
		return textureFileNameMap.contains(originalFilename);
	}

	std::mutex texMapMtx;

	~ResourceManager() {
		uploadCancel.store(true);
		asyncQueue.cancel_wait();
		if (asyncThread.joinable()) {
			asyncThread.join();
		}
	}

	void ReleaseFreeableTextures() {

		deletion_mtx.lock();

		for (size_t i = 0; i < deletionList.size(); i++)
		{
			if (deletionList[i].framesPassed >= 2 && deletionList[i].tex != nullptr) {
				FreeTexture(deletionList[i]);
				deletionList.erase(deletionList.begin() + i);
				i--;
			}
			else {
				deletionList[i].framesPassed++;
			}
		}
		deletion_mtx.unlock();
	}

	int ResourceCount() {
		return static_cast<int>(textureResources.size());
	}

	glm::ivec2 GetImageFileResolution(std::string filepath);

	framebufferID CreateFramebuffer(glm::ivec2 size, glm::vec4 clearColor, vk::Format format = framebufferImageFormat);
	DoubleFrameBufferContext* GetFramebuffer(framebufferID id);
	void ResizeFramebuffer(framebufferID framebuffer, glm::ivec2 size);
	void HandleFramebufferRecreation();

private:

	IDGenerator<framebufferID> fbIDGenerator;
	std::unordered_map<framebufferID, DoubleFrameBufferContext> framebufferRefMap;

	std::recursive_mutex  deletion_mtx;

	struct textureJob {
		std::string imagePath;
		FilterMode filtermode;
		texID id;
	};
	struct textureFreeable {
		texID id = 0;
		Texture* tex = nullptr;
		int framesPassed = 0; // swapchain synchronizations since queued
	};

	void FreeTexture(textureFreeable& t);


	void setTextureResource(texID id, Texture tex) {
		std::unique_lock<std::mutex> lock(texMapMtx);
		textureResources.insert({id, tex });
	}

	void uploadThreadFunc(ConcurrentQueue<textureJob>& asyncQueue);

	VKEngine::ThreadContext uploadContext; // for async uploading
	ConcurrentQueue<textureJob> asyncQueue;
	std::thread asyncThread;
	std::atomic<bool> uploadCancel = false;
	std::vector<textureFreeable> deletionList;

	bool slatedForDeletion(texID id) {
		deletion_mtx.lock();
		for (size_t i = 0; i < deletionList.size(); i++) {
			if (deletionList[i].id == id) {
				deletion_mtx.unlock();
				return true;
			}
		}
		deletion_mtx.unlock();
		return false;
	}

	// may request deletion before creation is complete
	void cancelDeletion(texID id) {
		deletion_mtx.lock();
		for (size_t i = 0; i < deletionList.size(); i++)
		{
			if (deletionList[i].id == id) {
				deletionList.erase(deletionList.begin() + i);
				deletion_mtx.unlock();
				return;
			}
		}
		deletion_mtx.unlock();
	}

	ChangeFlags* changeFlags = nullptr;
	robin_hood::unordered_node_map<texID, Texture> textureResources;

	robin_hood::unordered_flat_map<std::string, texID> textureFileNameMap;
	robin_hood::unordered_flat_map<texID, std::string> fileNameTextureMap;
	//IDGenerator<texID> TextureIDGenerator;
	
	SlotManager<texID> textureIDSlotManager;
	VKEngine* rengine;
};

