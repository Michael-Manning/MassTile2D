#include "stdafx.h"

#include <memory>

#include "tilemapPL.h"
#include <glm/glm.hpp>
#include "typedefs.h"
#include "TileWorld.h"
#include "Player.h"

#include "Engine.h"

namespace {
	Engine* _engine;
}


glm::vec2 gameSceneWorldToScreenPos(glm::vec2 pos) {
#ifdef USING_EDITOR
	if (showingEditor)
		return editor.gameSceneWorldToScreenPos(pos);
#endif
	return worldToScreenPos(pos, mainCamera, _engine->getWindowSize());
}
glm::vec2 gameSceneSreenToWorldPos(glm::vec2 pos) {
#ifdef USING_EDITOR
	if (showingEditor)
		return editor.gameSceneSreenToWorldPos(pos);
#endif
	return screenToWorldPos(pos, mainCamera, _engine->getWindowSize());
}




namespace global {

	void SetEngine(Engine* engine) {
		_engine = engine;
	}

	Player* player = nullptr;
	Scene* mainScene = nullptr;
	AssetManager* assetManager = nullptr;
	Input* input = nullptr;
	InventoryContainer playerInventory = InventoryContainer(PlayerInventorySize);
	InventoryContainer cursorInventory = InventoryContainer(1);
	InventoryContainer* inspectedInventory = nullptr; 
	glm::vec2 inspectedInventoryLocation;
	
	TileWorld* tileWorld = nullptr;
}