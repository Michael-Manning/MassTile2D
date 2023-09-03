#pragma once


#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <memory>
#include <string>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include<box2d/box2d.h>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"
#include "coloredQuadPL.h"
#include "texturedQuadPL.h"
#include "tilemapPL.h"
#include "IDGenerator.h"
#include "typedefs.h"
#include "Physics.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "ECS.h"
#include "Input.h"
#include "Scene.h"
#include "vulkan_util.h"
#include "globalBufferDefinitions.h"
#include "TileWorld.h"


#ifdef NDEBUG

#define DebugLog(msg) ((void)0)

#else
#include <Windows.h>
static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#define DebugLog(msg) SetConsoleTextAttribute(hConsole, 10); std::cout << "DEBUG: " << (msg) << std::endl;  SetConsoleTextAttribute(hConsole, 15)
#endif


struct debugStats {
	int sprite_render_count = 0;
	int entity_count = 0;
};

class Engine {

public:

	Camera camera{
		glm::vec2(0.0f),
		1.0f
	};


	Engine(std::shared_ptr<VKEngine> rengine, AssetManager::AssetPaths assetPaths) : rengine(rengine),
		assetManager(std::make_shared<AssetManager>(rengine, assetPaths))
	{

		DebugLog("Engine created");

		scene = std::make_shared<Scene>(assetManager);
	}

	void Start(std::string windowName, int winW, int winH, std::string shaderDir);

	void loadPrefabs();

	bool ShouldClose();
	void Close();

	bool QueueNextFrame();

	void EntityStartUpdate() {
		if (paused)
			return;

		Entity::DeltaTime = deltaTime;
		for (auto& i : scene->sceneData.entities) {
			i.second->_runStartUpdate();
		}
	};

	std::shared_ptr<Input> GetInput() {
		return input;
	}

	void hintWindowResize() {
		rengine->framebufferResized = true;
	};

	glm::vec2 getWindowSize() {
		return glm::vec2(winW, winH);
	};

	std::shared_ptr<Scene> scene;

	std::shared_ptr<b2World> bworld = nullptr;

	bool paused = true;
	double deltaTime = 0.0;
	double paused_deltaTime = 0.0; // delta time, but unaffected by pausing
	double framerate;
	float time = 0.0f;

	std::shared_ptr<AssetManager> assetManager;

	// convert from normalized coordinates to pixels from top left
	glm::vec2 worldToScreenPos(glm::vec2 pos) {
		pos *= glm::vec2(1.0f, -1.0f);
		pos += glm::vec2(-camera.position.x, camera.position.y);
		pos *= camera.zoom;
		pos += glm::vec2((float)winW / winH, 1.0f);
		pos *= glm::vec2(winH / 2.0f);
		return pos;
	}

	glm::vec2 screenToWorldPos(glm::vec2 pos) {

		pos /= glm::vec2(winH / 2.0f);
		pos -= glm::vec2((float)winW / winH, 1.0f);
		pos /= camera.zoom;
		pos -= glm::vec2(-camera.position.x, camera.position.y);
		pos /= glm::vec2(1.0f, -1.0f);

		return pos;
	}

	debugStats& _getDebugStats() {
		return runningStats;
	}

	void setTilemapAtlasTexture(texID texture) {
		assert(tilemapPipeline->textureAtlas.has_value() == false);
		tilemapPipeline->setTextureAtlas(assetManager->textureAssets[texture]);
	};

	int winW = 0, winH = 0;

	void _onWindowResize() {
		// used to update piplines. use for something else or delete
	};






	std::shared_ptr<TileWorld> worldMap = nullptr;

private:

	VertexMeshBuffer quadMeshBuffer;

	double lastTime = 0.0;

	double physicsTimer = 0.0;


	std::shared_ptr<VKEngine> rengine = nullptr;

	Texture texNotFound; // displayed when indexing incorrectly 

	std::unique_ptr<TilemapPL> tilemapPipeline = nullptr;
	std::unique_ptr<ColoredQuadPL> colorPipeline = nullptr;
	std::unique_ptr<TexturedQuadPL> instancedPipeline = nullptr;

	VKUtil::UBOUploader<cameraUBO_s> cameraUploader;

	uint32_t frameCounter = 0;
	bool firstFrame = true;

	std::shared_ptr<Input> input;

	debugStats runningStats = { 0 };

	void initPhysics();
	void updatePhysics();
	void InitImgui();
};
