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
#include "LightingComputePL.h"
#include "TextPL.h"

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
#include "Settings.h"
#include "AssetManager.h"
#include "ResourceManager.h"

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


	Engine(
		std::shared_ptr<VKEngine> rengine, 
		AssetManager::AssetPaths assetPaths)
		: 
		rengine(rengine),
		resourceManager(std::make_shared<ResourceManager>(rengine)),
		assetChangeFlags(std::make_shared<AssetManager::ChangeFlags>())
	{
		assetManager = std::make_shared<AssetManager>(rengine, assetPaths, resourceManager, assetChangeFlags);
		bworld = std::make_shared<b2World>(gravity);
		currentScene = std::make_shared<Scene>(bworld);

		DebugLog("Engine created");
	}

	void Start(const VideoSettings& initialSettings);
	void ApplyNewVideoSettings(const VideoSettings settings);

	bool ShouldClose();
	void Close();

	bool QueueNextFrame(bool drawImgui);

	void EntityStartUpdate() {
		if (paused)
			return;

		Entity::DeltaTime = deltaTime;
		for (auto& i : currentScene->sceneData.entities) {
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

	float _getAverageFramerate() {
		float sum = 0;
		for (auto& f : frameTimes)
			sum += f;
		return sum / (float)frameTimeBufferCount;
	}

	void setTilemapAtlasTexture(texID texture) {
		assert(tilemapPipeline->textureAtlas.has_value() == false);
		tilemapPipeline->setTextureAtlas(resourceManager->GetTexture(texture));
	};

	int winW = 0, winH = 0;

	void _onWindowResize() {
		screenSpaceTransformUploader.Invalidate();
	};

	std::shared_ptr<TileWorld> worldMap = nullptr;

	inline void addScreenSpaceQuad(glm::vec4 color, glm::vec2 pos, glm::vec2 scale) {
		ColoredQuadPL::InstanceBufferData item;
		item.color = color;
		item.position = pos;
		item.scale = scale;
		item.rotation = 0.0f;
		item.circle = 0;
		screenSpaceColorDrawlist.push_back(item);
	}


	inline void addScreenSpaceText(fontID font, glm::vec2 position, glm::vec4 color, std::string text) {
		screenSpaceTextDrawItem item;
		item.font = font;
		item.text = text;
		item.header.color = color;
		item.header.position = position;
		item.header.rotation = 0.0f;
		item.header.textLength = text.length();
		screenSpaceTextDrawlist.push_back(item);
	};

	void addScreenSpaceText(fontID font, glm::vec2 position, glm::vec4 color, const char* fmt, ...) {

		char buffer[TEXTPL_maxTextLength];

		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, sizeof(buffer), fmt, args);
		va_end(args);

		std::string result = buffer;

		screenSpaceTextDrawItem item;
		item.font = font;
		item.text = result;
		item.header.color = color;
		item.header.position = position;
		item.header.rotation = 0.0f;
		item.header.textLength = result.length();
		screenSpaceTextDrawlist.push_back(item);
	};

	void clearScreenSpaceDrawlist() {
		screenSpaceColorDrawlist.clear();
		screenSpaceTextDrawlist.clear();
	}

	void SetScene(std::shared_ptr<Scene> scene) {
		currentScene = scene;
	}
	std::shared_ptr<Scene> GetCurrentScene() {
		return currentScene;
	}

private:

	std::shared_ptr<Scene> currentScene = nullptr;
	std::shared_ptr<AssetManager::ChangeFlags> assetChangeFlags;
	std::shared_ptr<ResourceManager> resourceManager = nullptr;
	std::vector<ColoredQuadPL::InstanceBufferData> screenSpaceColorDrawlist;

	struct screenSpaceTextDrawItem {
		TextPL::textHeader header;
		std::string text;
		fontID font;
		float scaleFactor = 1.0f;
	};
	std::vector<screenSpaceTextDrawItem> screenSpaceTextDrawlist;


	VertexMeshBuffer quadMeshBuffer;

	double lastTime = 0.0;

	double physicsTimer = 0.0;


	std::shared_ptr<VKEngine> rengine = nullptr;

	Texture texNotFound; // displayed when indexing incorrectly 

	std::unique_ptr<TilemapPL> tilemapPipeline = nullptr;
	std::unique_ptr<ColoredQuadPL> colorPipeline = nullptr;
	std::unique_ptr<TexturedQuadPL> texturePipeline = nullptr;
	std::unique_ptr<TextPL> textPipeline = nullptr;
	std::unique_ptr<LightingComputePL> lightingPipeline = nullptr;

	std::unique_ptr<ColoredQuadPL> screenSpaceColorPipeline = nullptr;
	std::unique_ptr<TextPL> screenSpaceTextPipeline = nullptr;

	VKUtil::BufferUploader<cameraUBO_s> cameraUploader;
	VKUtil::BufferUploader<cameraUBO_s> screenSpaceTransformUploader;

	uint32_t frameCounter = 0;
	bool firstFrame = true;

	std::shared_ptr<Input> input;

	debugStats runningStats = { 0 };

	int frameTimeIndex = 0;
	const static int frameTimeBufferCount = 128;
	float frameTimes[frameTimeBufferCount];

	void initPhysics();
	void updatePhysics();
	void InitImgui();

	// physics settings
	const b2Vec2 gravity = b2Vec2(0.0f, -10.0f);
	const double timeStep = 1.0f / 60.0f;
	const int32 velocityIterations = 6;
	const int32 positionIterations = 2;

	bool newVideoSettingsRequested = false;
	VideoSettings requestedSettings;
};
