#pragma once

#include <memory>
#include <glm/glm.hpp>
#include"Engine.h"
#include "typedefs.h"


// TODO: store engine in class instead of passing it all over the place

class Editor {
public:

	void Run();

	void Initialize(
		Engine* engine, 
		std::shared_ptr<Scene> gameScene, 
		sceneRenderContextID sceneRenderContext, 
		std::function<void(std::shared_ptr<Scene>)> onMainSceneLoad);

	std::vector<Engine::SceneRenderJob> GetAdditionalRenderJobs() {
		std::vector<Engine::SceneRenderJob> jobs;

		if (showingPreviewWindow) {
			entityPreviewRenderJob.camera = previewCamera;
			jobs.push_back(entityPreviewRenderJob);
		}

		return jobs;
	}

	void SetGameScene(std::shared_ptr<Scene> scene) {
		this->gameScene = scene;
	};

	bool showingCreateWindow = false;

	Camera editorCamera;

private:

	Engine* engine;

	std::function<void(std::shared_ptr<Scene>)> setSceneCallback;

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
	bool drawInspector(T& comp);

	template<>
	bool drawInspector<Transform>(Transform& t);

	template<>
	bool drawInspector<ColorRenderer>(ColorRenderer& t);

	template<>
	bool drawInspector<SpriteRenderer>(SpriteRenderer& r);

	template<>
	bool drawInspector<TextRenderer>(TextRenderer& r);

	bool drawInspector(ParticleSystemRenderer& r);

	template<>
	bool drawInspector<Rigidbody>(Rigidbody& r);

	template<>
	bool drawInspector<Staticbody>(Staticbody& r);

	void DrawGameSceneGrid(ImDrawList* drawlist, glm::vec2 size, glm::vec2 offset);
	void DrawPreviewSceneGrid(ImDrawList* drawlist, glm::vec2 size, glm::vec2 offset);
	void controlWindow();
	void entityWindow();
	void assetWindow();
	void mainSceneWindow();
	void EntityPreviewWindow();

	glm::vec2 DrawSpriteAtlas(spriteID id, glm::vec2 maxSize, int atlasIndex);
	glm::vec2 DrawSprite(spriteID id, glm::vec2 maxSize);

	bool showingStats = false;
	void debugDataWindow();

	void EntitySelectableTree(int& index, Entity* entity);

	bool behaviorModel = true;

	bool showingPreviewWindow = false;

	bool assetModel = true;
	bool fontModel = false;
	spriteID rendererSelectedSprite = 0;

	int comboSelected = -1;
	int fontComboSelected = -1;

	int selectedEntityIndex = -1;
	Entity* selectedEntity = nullptr;

	int selectedPrefabIndex = 0;
	int selectedSceneIndex = 0;

	void clearInspectorSelection() {
		selectedEntityIndex = -1;
		selectedEntity = nullptr;

		rendererSelectedSprite = 0;

		sceneSelected = false;
	}

	int selectedSpriteIndex = -1;
	int selectedSpriteAtlasIndex = 0;
	Sprite* selectedSprite = nullptr;

	bool sceneSelected = false;

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