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

#include<vulkan/vulkan.hpp>

#include "VKEngine.h"
#include "typedefs.h"
#include "Texture.h"
#include "IDGenerator.h"
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
		uploadContext(engine->GenerateThreadContext_gfx(true))
	{
		asyncThread = std::thread(&ResourceManager::uploadThreadFunc, this, std::ref(asyncQueue));
	};

	Texture * GetTexture(texID id) {
		std::unique_lock<std::mutex> lock(texMapMtx);
		if (textureResources.contains(id) == false)
			return nullptr;
		return &textureResources[id];
	}

	const std::unordered_map<texID, Texture>* GetInternalTextureResources() {
		return &textureResources;
	}

	//auto _getTextureIterator() { return MapProxy<texID, std::shared_ptr<Texture>>(textureResources.begin(), textureResources.end()); };

	texID GenerateTexture(int w, int h, std::vector<uint8_t>& data, FilterMode filterMode, bool imGuiTexure = true);
	texID LoadTexture(std::string imagePath, FilterMode filterMode, bool imGuiTexure = true);
	texID LoadTexture(uint8_t* imageFileData, int dataLength, std::string fileName, FilterMode filterMode, bool imGuiTexure);

	texID LoadTextureAsync(std::string imagePath, FilterMode filterMode);

	void UpdateTexture(texID id, FilterMode filterMode);

	// will delete the texture once it is not in use
	void SetTextureFreeable(texID id) {

		deletion_mtx.lock();

		// given that textures freeable tracks frame synchnronizations, it would be a mistake
		// to call this function from a different thread than the engine thread.
		// This assertion may be removed if additional synchronization is added
		assert(std::this_thread::get_id() == rengine->defaultThreadID);
		//assert(textureResources.contains(id));

		for (auto& t : deletionList)
		{
			assert(id != t.id);
		}

		textureFreeable t;
		t.id = id;

		{
			std::unique_lock<std::mutex> lock(texMapMtx);
			if (textureResources.contains(id))
				t.tex = &textureResources[id];

			textureResources.erase(id);
		}
		TextureIDGenerator.Erase(t.id);
		deletionList.push_back(t);
		deletion_mtx.unlock();
	}

	bool HasTexture(texID id) {
		std::unique_lock<std::mutex> lock(texMapMtx);
		return textureResources.contains(id);
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

	framebufferID CreateFramebuffer(glm::ivec2 size, glm::vec4 clearColor);
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

	void FreeTexture(textureFreeable& t) {
		assert(t.tex != nullptr);
		if (t.tex->imTexture.has_value())
			ImGui_ImplVulkan_RemoveTexture(t.tex->imTexture.value());
		rengine->freeTexture(*t.tex);
	};


	void setTextureResource(texID id, Texture tex) {
		std::unique_lock<std::mutex> lock(texMapMtx);
		textureResources.insert(std::pair<texID, Texture>(id, tex));
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
	std::unordered_map<texID, Texture> textureResources; // reserve capacity or replace with collection protected against re-hashing due to pointer use
	IDGenerator<texID> TextureIDGenerator;
	VKEngine* rengine;
};

