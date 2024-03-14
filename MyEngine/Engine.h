#pragma once

#include <vector>
#include <stdint.h>
#include <memory>
#include <string>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "texture.h"
#include "VKEngine.h"

#include "pipelines.h"

#include "IDGenerator.h"
#include "typedefs.h"
#include "Camera.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "Input.h"
#include "Scene.h"
#include "globalBufferDefinitions.h"
#include "TileWorld.h"
#include "Settings.h"
#include "AssetManager.h"
#include "ResourceManager.h"
#include "GlobalImageDescriptor.h"
#include "Drawlist.h"

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

private:
	std::unique_ptr<VKEngine> rengine = nullptr;

	class DrawlistGraphicsContext {
	public:

		DrawlistGraphicsContext(DrawlistAllocationConfiguration allocationSettings)
			: allocationSettings(allocationSettings) {}

		const DrawlistAllocationConfiguration allocationSettings;

		std::unique_ptr<ColoredQuadPL> coloredQuadPipeline = nullptr;
		std::unique_ptr<ColoredTrianglesPL> coloredTrianglesPipeline = nullptr;
		std::unique_ptr<TexturedQuadPL> texturedQuadPipeline = nullptr;
		std::unique_ptr<TextPL> textPipeline = nullptr;
	};

	class SceneGraphicsContext {
	public:

		SceneGraphicsContext(SceneGraphicsAllocationConfiguration allocationSettings)
			: allocationSettings(allocationSettings) {}

		const SceneGraphicsAllocationConfiguration allocationSettings;

		MappedDoubleBuffer<coodinateTransformUBO_s> cameraBuffers;

		std::unique_ptr<LightingComputePL> lightingPipeline = nullptr;

		std::unique_ptr<TilemapPL> tilemapPipeline = nullptr;
		std::unique_ptr<TilemapLightRasterPL> tilemapLightRasterPipeline = nullptr;
		std::unique_ptr<ColoredQuadPL> colorPipeline = nullptr;
		std::unique_ptr<TexturedQuadPL> texturePipeline = nullptr;
		std::unique_ptr<TextPL> textPipeline = nullptr;
		std::unique_ptr<ParticleSystemPL> particlePipeline = nullptr;

		std::unique_ptr<TileWorld> worldMap = nullptr;

		texID tilemapTextureAtlas;

		framebufferID drawFramebuffer;
		framebufferID lightingFramebuffer;
	};

public:

	struct SceneRenderJob {
		std::shared_ptr<Scene> scene = nullptr;
		Camera camera = {};
		sceneGraphicsContextID sceneRenderCtxID = 0;
	};


	Engine() :
		rengine(std::make_unique<VKEngine>()),
		resourceChangeFlags(std::make_unique<ResourceManager::ChangeFlags>()),
		GlobalTextureDesc(rengine.get())
	{
		DebugLog("Engine created");
	}

	~Engine() {
	}

	void Start(const VideoSettings& initialSettings, AssetManager::AssetPaths assetPaths, EngineMemoryAllocationConfiguration allocationSettings);
	void ShowWindow();
	void ApplyNewVideoSettings(const VideoSettings settings);

	bool ShouldClose();
	void Close();

	bool QueueNextFrame(const std::vector<SceneRenderJob>& sceneRenderJobs, bool drawImgui);

	void EntityStartUpdate(std::shared_ptr<Scene> scene) {
		if (scene->paused)
			return;

		if (scene->paused)
			Behaviour::deltaTime = 0.0f;
		else
			Behaviour::deltaTime = deltaTime;

		scene->ProcessDeferredDeletions(deltaTime);
		for (auto& [id, behaviour] : scene->sceneData.behaviours) {
			behaviour->_runStartUpdate();
		}
	};

	Input* GetInput() {
		return input.get();
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


	double deltaTime = 0.0;
	//double paused_deltaTime = 0.0; // delta time, but unaffected by pausing
	double framerate;
	float time = 0.0f;

	std::unique_ptr<AssetManager> assetManager;


	debugStats& _getDebugStats() {
		return runningStats;
	}

	float _getAverageFramerate() {
		float sum = 0;
		for (auto& f : frameTimes)
			sum += f;
		return sum / (float)frameTimeBufferCount;
	}

	void setTilemapAtlasTexture(sceneGraphicsContextID contextID, texID texture) {
		assert(sceneRenderContextMap.at(contextID).allocationSettings.AllocateTileWorld);
		sceneRenderContextMap.at(contextID).tilemapTextureAtlas = texture;
	};

	int winW = 0, winH = 0;

	void _onWindowResize() {
		//screenSpaceTransformUploader.Invalidate();
	};

	sceneGraphicsContextID CreateSceneRenderContext(glm::ivec2 framebufferSize, SceneGraphicsAllocationConfiguration allocationSettings) {

		sceneGraphicsContextID id = renderContextGenerator.GenerateID();
		auto [iter, inserted] = sceneRenderContextMap.emplace(id, allocationSettings);

		initializeSceneGraphicsContext(iter->second, framebufferSize);

		return id;
	}

	void ResizeSceneRenderContext(sceneGraphicsContextID id, glm::ivec2 size) {
		auto& ctx = sceneRenderContextMap.at(id);
		resourceManager->ResizeFramebuffer(ctx.drawFramebuffer, size);
		if (ctx.allocationSettings.AllocateTileWorld)
			resourceManager->ResizeFramebuffer(ctx.lightingFramebuffer, size);

	}

	framebufferID GetSceneRenderContextFramebuffer(sceneGraphicsContextID id) {
		return sceneRenderContextMap.find(id)->second.drawFramebuffer;
	}

	framebufferID _GetSceneRenderContextLightMapBuffer(sceneGraphicsContextID id) {
		return sceneRenderContextMap.find(id)->second.lightingFramebuffer;
	}

	glm::ivec2 GetFramebufferSize(framebufferID id) {
		return resourceManager->GetFramebuffer(id)->targetSize;
	}

	TileWorld* GetSceneRenderContextTileWorld(sceneGraphicsContextID id) {
		assert(sceneRenderContextMap.at(id).allocationSettings.AllocateTileWorld);
		return sceneRenderContextMap.at(id).worldMap.get();
	}

	glm::vec4 GetFramebufferClearColor(framebufferID id) {
		return resourceManager->GetFramebuffer(id)->clearColor;
	}
	void SetFramebufferClearColor(framebufferID id, glm::vec4 color) {
		resourceManager->GetFramebuffer(id)->clearColor = color;
	}

	void ImGuiFramebufferImage(framebufferID framebuffer, glm::ivec2 displaySize) {
		auto fb = resourceManager->GetFramebuffer(framebuffer);
		auto tex = fb->textures[rengine->currentFrame];
		ImGui::Image((ImTextureID)tex->imTexture.value(), ImVec2(displaySize.x, displaySize.y));
	};

	Drawlist* GetScreenspaceDrawlist(int layer = 0) {
		assert(screenspaceDrawlistLayers.size() >= layer);
		return &screenspaceDrawlistLayers[layer];
	}

private:

	DrawlistGraphicsContext createDrawlistGraphicsContext(DrawlistAllocationConfiguration allocationSettings, MappedDoubleBuffer<coodinateTransformUBO_s> cameraBuffers);

	const vk::Format lightingPassFormat = vk::Format::eR16Unorm; //vk::Format::eR16Unorm;

	GlobalImageDescriptor GlobalTextureDesc;
	std::array<bool, FRAMES_IN_FLIGHT> textureDescriptorDirtyFlags = { false, false };

	std::queue<texID> textureBindingDeletionQueue;
	std::unique_ptr<ResourceManager::ChangeFlags> resourceChangeFlags;
	std::unique_ptr<ResourceManager> resourceManager = nullptr;

	IDGenerator<sceneGraphicsContextID> renderContextGenerator;

	std::unordered_map <sceneGraphicsContextID, SceneGraphicsContext> sceneRenderContextMap;

	VertexMeshBuffer quadMeshBuffer;

	double lastTime = 0.0;

	int lastwinW, lastwinH;
	bool windowResizedLastFrame = false;

	texID texNotFoundID; // displayed when indexing incorrectly 


	void initializeSceneGraphicsContext(SceneGraphicsContext& ctx, glm::ivec2 framebufferSize);

	void recordSceneContextGraphics(const SceneGraphicsContext& ctx, std::shared_ptr<Scene> scene, const Camera& camera, vk::CommandBuffer& cmdBuffer);

	void recordDrawlistContextGraphics(const DrawlistGraphicsContext& ctx, Drawlist& drawData, vk::CommandBuffer cmdBuffer);

	void bindQuadMesh(vk::CommandBuffer cmdBuffer) {
		vk::Buffer vertexBuffers[] = { quadMeshBuffer.vertexBuffer };
		vk::DeviceSize offsets[] = { 0 };
		cmdBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
		cmdBuffer.bindIndexBuffer(quadMeshBuffer.indexBuffer, 0, vk::IndexType::eUint16);
	}

	std::array<ComponentResourceToken, MAX_PARTICLE_SYSTEMS_LARGE> particleSystemResourceTokens;
	std::unique_ptr<ParticleComputePL> particleComputePipeline = nullptr;

	std::vector<DrawlistGraphicsContext> screenspaceDrawlistContexts;
	std::vector<Drawlist> screenspaceDrawlistLayers;

	MappedDoubleBuffer<coodinateTransformUBO_s> screenSpaceTransformCameraBuffer;
	//VKUtil::BufferUploader<coodinateTransformUBO_s> screenSpaceTransformUploader; // TODO: replace with plain mapped double buffer

	uint32_t frameCounter = 0;
	bool firstFrame = true;

	std::unique_ptr<Input> input;

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

	DeviceBuffer computerParticleBuffer;
};
