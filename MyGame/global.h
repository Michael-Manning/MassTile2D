#pragma once

#include <memory>

#include "tilemapPL.h"
#include <glm/glm.hpp>
#include "typedefs.h"
#include "TileWorld.h"
#include "Player.h"
#include "Inventory.h"


#ifdef USING_EDITOR
#include "Editor.h"
extern Editor editor;
extern bool showingEditor;
#else
extern const bool showingEditor;
#endif 

extern Camera mainCamera;

glm::vec2 gameSceneWorldToScreenPos(glm::vec2 pos);

glm::vec2 gameSceneSreenToWorldPos(glm::vec2 pos);



extern TileWorld* tileWolrdGlobalRef;

extern glm::vec2 GcameraPos; // remove

class Player;
class Engine;

namespace global {

	void SetEngine(Engine* engine);

	extern Player* player;

	extern Scene* mainScene;

	extern AssetManager* assetManager;

	extern InventoryContainer playerInventory;
	extern InventoryContainer cursorInventory;
}