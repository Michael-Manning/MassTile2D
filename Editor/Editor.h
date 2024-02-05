#pragma once

#include <memory>
#include <glm/glm.hpp>
#include"Engine.h"
#include "typedefs.h"


// TODO: store engine in class instead of passing it all over the place

class Editor {
public:

	void Run(Engine& engine);

	void Initialize(Engine& engine, std::shared_ptr<Scene> gameScene, sceneRenderContextID sceneRenderContext);

	std::vector<Engine::SceneRenderJob> GetAdditionalRenderJobs() {
		std::vector<Engine::SceneRenderJob> jobs;

		if (showingPreviewWindow) {
			entityPreviewRenderJob.camera = previewCamera;
			jobs.push_back(entityPreviewRenderJob);
		}

		return jobs;
	}


	bool showingCreateWindow = false;

	Camera editorCamera;

private:

	Camera previewCamera;
	std::shared_ptr<Scene> entityPreviewScene;
	Engine::SceneRenderJob entityPreviewRenderJob;
	sceneRenderContextID entityPreviewsSeneRenderContextID;
	glm::vec2 entityPrviewFrameSize;
	framebufferID entityPreviewFramebuffer;
	glm::vec2 previewSceneViewerScreenLocation;

	std::shared_ptr<Scene> gameScene = nullptr;
	sceneRenderContextID sceneRenderContext;
	framebufferID sceneFramebuffer;

	template<typename T>
	bool drawInspector(T& comp, Engine& engine);

	template<>
	bool drawInspector<Transform>(Transform& t, Engine& engine);

	template<>
	bool drawInspector<ColorRenderer>(ColorRenderer& t, Engine& engine);

	template<>
	bool drawInspector<SpriteRenderer>(SpriteRenderer& r, Engine& engine);

	template<>
	bool drawInspector<TextRenderer>(TextRenderer& r, Engine& engine);

	bool drawInspector(ParticleSystemRenderer& r, Engine& engine);

	template<>
	bool drawInspector<Rigidbody>(Rigidbody& r, Engine& engine);

	template<>
	bool drawInspector<Staticbody>(Staticbody& r, Engine& engine);

	void DrawGameSceneGrid(Engine& engine, ImDrawList* drawlist, glm::vec2 size, glm::vec2 offset);
	void DrawPreviewSceneGrid(Engine& engine, ImDrawList* drawlist, glm::vec2 size, glm::vec2 offset);
	void controlWindow(Engine& engine);
	void entityWindow(Engine& engine);
	void assetWindow(Engine& engine);
	void mainSceneWindow(Engine& engine);
	void EntityPreviewWindow(Engine& engine);

	glm::vec2 DrawSpriteAtlas(Engine& engine, spriteID id, glm::vec2 maxSize, int atlasIndex);
	glm::vec2 DrawSprite(Engine& engine, spriteID id, glm::vec2 maxSize);

	bool showingStats = false;
	void debugDataWindow(Engine& engine);

	bool behaviorModel = true;

	bool showingPreviewWindow = false;

	bool assetModel = true;
	bool fontModel = false;
	spriteID rendererSelectedSprite = 0;

	int comboSelected = -1;
	int fontComboSelected = -1;

	int selectedEntityIndex = 0;
	std::shared_ptr<Entity> selectedEntity = nullptr;

	int selectedPrefabIndex = 0;

	int selectedSpriteIndex = -1;
	int selectedSpriteAtlasIndex = 0;
	std::shared_ptr<Sprite> selectedSprite = nullptr;

	float updateTimer = 0.0f;
	float frameRateStat;

	glm::vec2 lastMpos = glm::vec2(0);
	glm::vec2 mouseDelta = glm::vec2(0);

	bool draggingY = false;
	bool draggingX = false;
	bool draggingAngle = false;
	float dragInitialAngle = 0.0f;
	float dragInitialObjectAngle = 0.0f;

	bool showGrid = true;

	float mainSceneRawZoom = 0.4f;
	float previewSceneRawZoom = 0.4f;

	glm::vec2 camSlerpStart;
	glm::vec2 camSlerpEnd;
	float cameraSlepStartTime;
	bool cameraEntityFocus = false;

	std::shared_ptr<Input> input;

	// set every frame, read by different functions
	glm::vec2 screenSize;
	glm::vec2 mainSceneFrameSize;
	glm::vec2 lastMainSceneFrameSize;
	glm::vec2 mainSceneViewerScreenLocation; // position of view window framebuffer image on screen
	bool mainSceneFrameSizeChanged;
	/*ImDrawList* sceneViewDrawlist;*/


	// TODO: if rendering scene in an imgui window, must store screen space offset of the image render location and apply to these functions

	glm::vec2 gameSceneWorldToScreenPos(glm::vec2 pos) {
		return worldToScreenPos(pos, editorCamera, mainSceneFrameSize) + mainSceneViewerScreenLocation;
	}

	glm::vec2 gameSceneSreenToWorldPos(glm::vec2 pos) {
		return screenToWorldPos(pos - mainSceneViewerScreenLocation, editorCamera, mainSceneFrameSize);
	}

	glm::vec2 previewSceneWorldToScreenPos(glm::vec2 pos) {
		return worldToScreenPos(pos, previewCamera, entityPrviewFrameSize) + previewSceneViewerScreenLocation;
	}

	glm::vec2 previewSceneSreenToWorldPos(glm::vec2 pos) {
		return screenToWorldPos(pos - previewSceneViewerScreenLocation, previewCamera, entityPrviewFrameSize);
	}
};