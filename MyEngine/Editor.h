#pragma once

#include <memory>
#include <glm/glm.hpp>
#include"Engine.h"


class Editor {
public:

	void Run(Engine& engine);

	template<typename T>
	bool drawInspector(T& comp, Engine& engine);

	template<>
	bool drawInspector<Transform>(Transform& t, Engine& engine);

	template<>
	bool drawInspector<ColorRenderer>(ColorRenderer& t, Engine& engine);
	
	template<>
	bool drawInspector<SpriteRenderer>(SpriteRenderer& r, Engine& engine);

	template<>
	bool drawInspector<Rigidbody>(Rigidbody& r, Engine& engine);

	template<>
	bool drawInspector<Staticbody>(Staticbody& r, Engine& engine);

	bool showingCreateWindow = false;

private:

	void DrawGrid(Engine& engine);
	void controlWindow(Engine& engine);
	void entityWindow(Engine& engine);
	void assetWindow(Engine& engine);

	bool showingStats = false;
	void debugDataWindow(Engine& engine);

	bool behaviorModel = true;

	bool assetModel = true;
	spriteID rendererSelectedSprite = 0;

	int comboSelected = -1;

	int selectedEntityIndex = 0;
	std::shared_ptr<Entity> selectedEntity = nullptr;

	int selectedPrefabIndex = 0;

	int selectedSpriteFilterCombo = 0;
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

	// set every frame, read by different functions
	glm::vec2 screenSize;
	ImDrawList* drawlist;
};

