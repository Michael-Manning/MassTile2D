#pragma once

#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <memory>
#include <string>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include<box2d/box2d.h>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"

#include "pipelines.h"

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
#include "GlobalImageDescriptor.h"

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


struct ScenePipelineContext {
	MappedDoubleBuffer<cameraUBO_s> cameraBuffers;

	std::unique_ptr<TilemapPL> tilemapPipeline = nullptr;
	std::unique_ptr<ColoredQuadPL> colorPipeline = nullptr;
	std::unique_ptr<TexturedQuadPL> texturePipeline = nullptr;
	std::unique_ptr<TextPL> textPipeline = nullptr;
	std::unique_ptr<ParticleSystemPL> particlePipeline = nullptr;
};


class Engine {

public:

	struct SceneRenderJob{
		std::shared_ptr<Scene> scene = nullptr;
		Camera camera = {};
		sceneRenderContextID sceneRenderCtxID = 0;
	};


	Engine(
		std::shared_ptr<VKEngine> rengine)
		:
		rengine(rengine),
		resourceChangeFlags(std::make_shared<ResourceManager::ChangeFlags>()),
		GlobalTextureDesc(rengine),
		globalTextureBindingManager(MAX_TEXTURE_RESOURCES)
	{
		//bworld = std::make_shared<b2World>(gravity);
		//currentScene = std::make_shared<Scene>(bworld);

		DebugLog("Engine created");
	}

	void Start(const VideoSettings& initialSettings, AssetManager::AssetPaths assetPaths);
	void ApplyNewVideoSettings(const VideoSettings settings);

	bool ShouldClose();
	void Close();

	bool QueueNextFrame(const std::vector<SceneRenderJob>& sceneRenderJobs, bool drawImgui);

	void EntityStartUpdate(std::shared_ptr<Scene> scene) {
		if (scene->paused)
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

	// window was recently resized
	bool WindowResizedLastFrame() {
		return windowResizedLastFrame;
	};


	//std::shared_ptr<b2World> bworld = nullptr;

	double deltaTime = 0.0;
	//double paused_deltaTime = 0.0; // delta time, but unaffected by pausing
	double framerate;
	float time = 0.0f;

	std::shared_ptr<AssetManager> assetManager;

	//Camera tmp_camera;

	//// convert from normalized coordinates to pixels from top left
	//glm::vec2 worldToScreenPos(glm::vec2 pos) {
	//	pos *= glm::vec2(1.0f, -1.0f);
	//	pos += glm::vec2(-tmp_camera.position.x, tmp_camera.position.y);
	//	pos *= tmp_camera.zoom;
	//	pos += glm::vec2((float)winW / winH, 1.0f);
	//	pos *= glm::vec2(winH / 2.0f);
	//	return pos;
	//}

	//glm::vec2 screenToWorldPos(glm::vec2 pos) {

	//	pos /= glm::vec2(winH / 2.0f);
	//	pos -= glm::vec2((float)winW / winH, 1.0f);
	//	pos /= tmp_camera.zoom;
	//	pos -= glm::vec2(-tmp_camera.position.x, tmp_camera.position.y);
	//	pos /= glm::vec2(1.0f, -1.0f);

	//	return pos;
	//}

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
		assert(false);
		//assert(tilemapPipeline->textureAtlas.has_value() == false);
		//tilemapPipeline->setTextureAtlas(resourceManager->GetTexture(texture));
	};

	int winW = 0, winH = 0;

	void _onWindowResize() {
		screenSpaceTransformUploader.Invalidate();
	};

	std::shared_ptr<TileWorld> worldMap = nullptr;

	inline void addScreenSpaceQuad(glm::vec4 color, glm::vec2 pos, glm::vec2 scale, float rotation = 0.0f) {
		ColoredQuadPL::InstanceBufferData item;
		item.color = color;
		item.position = pos;
		item.scale = scale;
		item.rotation = rotation;
		item.circle = 0;
		screenSpaceColorDrawlist.push_back(item);
	}

	inline void addScreenCenteredSpaceTexture(std::shared_ptr<Sprite> sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f) {
		TexturedQuadPL::ssboObjectInstanceData item;
		item.uvMin = glm::vec2(0.0f);
		item.uvMax = glm::vec2(1.0f);
		item.translation = pos;
		item.scale = glm::vec2(sprite->resolution.x / sprite->resolution.y * height, height);
		item.rotation = rotation;
		item.tex = sprite->textureID; 

		if (sprite->atlas.size() > 0) {
			auto atEntry = sprite->atlas[atlasIndex];
			item.uvMin = atEntry.uv_min;
			item.uvMax = atEntry.uv_max;
		}

		screenSpaceTextureDrawlist.push_back(item);
	}
	inline void addScreenCenteredSpaceTexture(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f){
		auto s = assetManager->GetSprite(sprID);
		addScreenCenteredSpaceTexture(s, atlasIndex, pos, height, rotation);
	}
	inline void addScreenCenteredSpaceTexture(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f) {
		auto s = assetManager->GetSprite(sprite);
		addScreenCenteredSpaceTexture(s, atlasIndex, pos, height, rotation);
	}
	inline void addScreenSpaceTexture(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f) {
		auto s = assetManager->GetSprite(sprID);
		addScreenCenteredSpaceTexture(s, atlasIndex, pos + (s->resolution / 2.0f) * (height / s->resolution.y), height, rotation);
	}
	inline void addScreenSpaceTexture(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f) {
		auto s = assetManager->GetSprite(sprite);
		addScreenCenteredSpaceTexture(s, atlasIndex, pos + (s->resolution / 2.0f) * (height / s->resolution.y), height, rotation);
	}

	inline void addScreenCenteredSpaceFramebufferTexture(framebufferID fbID, glm::vec2 pos, float height, float rotation = 0.0f) {

		auto fb = resourceManager->GetFramebuffer(fbID);

		float w = fb->extents[rengine->currentFrame].width;
		float h = fb->extents[rengine->currentFrame].height;

		TexturedQuadPL::ssboObjectInstanceData item;
		item.uvMin = glm::vec2(0.0f);
		item.uvMax = glm::vec2(1.0f, -1.0f); // I don't actually know why this has to be flipped
		item.translation = pos;
		item.scale = glm::vec2((w / h) * height, height);
		item.rotation = rotation;
		item.tex = fb->textureIDs[rengine->currentFrame];

		screenSpaceTextureDrawlist.push_back(item);
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
		screenSpaceTextureDrawlist.clear();
	}

	//void SetScene(std::shared_ptr<Scene> scene) {
	//	currentScene = scene;
	//}
	//std::shared_ptr<Scene> GetCurrentScene() {
	//	return currentScene;
	//}

	sceneRenderContextID CreateSceneRenderContext(glm::ivec2 size, glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), bool transparentFramebufferBlending = false) {

		auto fbID = resourceManager->CreateFramebuffer(size, clearColor);
		auto fb = resourceManager->GetFramebuffer(fbID);

		sceneRenderContextID id = renderContextGenerator.GenerateID();
		sceneRenderContextMap[id] = {};
		
		sceneRenderContextMap.find(id)->second.fb = fbID;

		createScenePLContext(&sceneRenderContextMap.find(id)->second.pl, fb->renderpass, transparentFramebufferBlending);

		return id;
	}
	void ResizeSceneRenderContext(sceneRenderContextID id, glm::ivec2 size) {
		resourceManager->ResizeFramebuffer(sceneRenderContextMap.find(id)->second.fb, size);
	}

	framebufferID GetSceneRenderContextFramebuffer(sceneRenderContextID id) {
		return sceneRenderContextMap.find(id)->second.fb;
	}

	glm::ivec2 GetFramebufferSize(framebufferID id) {
		return resourceManager->GetFramebuffer(id)->targetSize;
	}

	void ImGuiFramebufferImage(framebufferID framebuffer, glm::ivec2 displaySize) {
		auto fb = resourceManager->GetFramebuffer(framebuffer);
		auto tex = fb->textures[rengine->currentFrame];
		ImGui::Image((ImTextureID)tex->imTexture.value(), ImVec2(displaySize.x, displaySize.y));
	};

private:

	GlobalImageDescriptor GlobalTextureDesc;
	BindingManager<texID, Texture*> globalTextureBindingManager;

	//std::shared_ptr<Scene> currentScene = nullptr;
	std::queue<texID> textureBindingDeletionQueue;
	std::shared_ptr<ResourceManager::ChangeFlags> resourceChangeFlags;
	std::shared_ptr<ResourceManager> resourceManager = nullptr;
	std::vector<ColoredQuadPL::InstanceBufferData> screenSpaceColorDrawlist;
	std::vector<TexturedQuadPL::ssboObjectInstanceData> screenSpaceTextureDrawlist;

	struct screenSpaceTextDrawItem {
		TextPL::textHeader header;
		std::string text;
		fontID font;
		float scaleFactor = 1.0f;
	};
	std::vector<screenSpaceTextDrawItem> screenSpaceTextDrawlist;

	IDGenerator<sceneRenderContextID> renderContextGenerator;
	struct sceneFB_PL{
		framebufferID fb;
		ScenePipelineContext pl;
	};
	std::unordered_map <sceneRenderContextID, sceneFB_PL> sceneRenderContextMap;

	VertexMeshBuffer quadMeshBuffer;

	double lastTime = 0.0;

	int lastwinW, lastwinH;
	bool windowResizedLastFrame = false;

	std::shared_ptr<VKEngine> rengine = nullptr;

	texID texNotFoundID; // displayed when indexing incorrectly 

	std::unique_ptr<LightingComputePL> lightingPipeline = nullptr;

	std::array<ComponentResourceToken, MAX_PARTICLE_SYSTEMS_LARGE> particleSystemResourceTokens;

	void createScenePLContext(ScenePipelineContext* ctx, vk::RenderPass renderpass, bool transparentFramebufferBlending);

	void recordSceneContextGraphics(const ScenePipelineContext& ctx, framebufferID framebuffer, std::shared_ptr<Scene> scene, const Camera& camera, vk::CommandBuffer& cmdBuffer);

	ScenePipelineContext tmp_sceneContext;

	std::unique_ptr<ColoredQuadPL> screenSpaceColorPipeline = nullptr;
	std::unique_ptr<TexturedQuadPL> screenSpaceTexturePipeline = nullptr;
	std::unique_ptr<TextPL> screenSpaceTextPipeline = nullptr;

	//VKUtil::BufferUploader<cameraUBO_s> cameraUploader;
	VKUtil::BufferUploader<cameraUBO_s> screenSpaceTransformUploader;

	uint32_t frameCounter = 0;
	bool firstFrame = true;

	std::shared_ptr<Input> input;

	debugStats runningStats = { 0 };

	int frameTimeIndex = 0;
	const static int frameTimeBufferCount = 128;
	float frameTimes[frameTimeBufferCount];

	void InitImgui();

	// physics settings
	const double timeStep = 1.0f / 60.0f;
	const int32 velocityIterations = 6;
	const int32 positionIterations = 2;

	bool newVideoSettingsRequested = false;
	VideoSettings requestedSettings;
	
	VmaAllocation deviceParticleBuffersAllocation;
};
