#pragma once

#include <memory>
#include <glm/glm.hpp>
#include"Engine.h"
#include "typedefs.h"


class Editor {
public:

	void Run(Engine& engine);

	void SetGameScene(std::shared_ptr<Scene> gameScene, sceneRenderContextID sceneRenderContext) {
		this->gameScene = gameScene;
		this->sceneRenderContext = sceneRenderContext;
	};


	bool showingCreateWindow = false;
	
	Camera editorCamera;

private:

	std::shared_ptr<Scene> gameScene = nullptr;
	sceneRenderContextID sceneRenderContext;

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

	template<>
	bool drawInspector<Rigidbody>(Rigidbody& r, Engine& engine);

	template<>
	bool drawInspector<Staticbody>(Staticbody& r, Engine& engine);

	void DrawGrid(Engine& engine);
	void controlWindow(Engine& engine);
	void entityWindow(Engine& engine);
	void assetWindow(Engine& engine);

	glm::vec2 DrawSpriteAtlas(Engine& engine, spriteID id, glm::vec2 maxSize, int atlasIndex);
	glm::vec2 DrawSprite(Engine& engine, spriteID id, glm::vec2 maxSize);

	bool showingStats = false;
	void debugDataWindow(Engine& engine);

	bool behaviorModel = true;

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

	bool draggingY = false;
	bool draggingX = false;
	bool draggingAngle = false;
	float dragInitialAngle = 0.0f;
	float dragInitialObjectAngle = 0.0f;

	bool showGrid = true;

	bool allowZoomThisFrame = true;
	float zoomP = 0.4f;

	glm::vec2 camSlerpStart;
	glm::vec2 camSlerpEnd;
	float cameraSlepStartTime;
	bool cameraEntityFocus = false;

	// set every frame, read by different functions
	glm::vec2 screenSize;
	glm::vec2 mainSceneFrameSize;
	glm::vec2 lastMainSceneFrameSize;
	glm::vec2 mainSceneViewerScreenLocation; // position of view window framebuffer image on screen
	ImDrawList* sceneViewDrawlist;


	// TODO: if rendering scene in an imgui window, must store screen space offset of the image render location and apply to these functions

	glm::vec2 gameSceneWorldToScreenPos(glm::vec2 pos) {
		return worldToScreenPos(pos, editorCamera, mainSceneFrameSize) + mainSceneViewerScreenLocation;
	}

	glm::vec2 gameSceneSreenToWorldPos(glm::vec2 pos) {
		return screenToWorldPos(pos - mainSceneViewerScreenLocation, editorCamera, mainSceneFrameSize);
	}
};