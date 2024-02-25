#pragma once

#include <memory>

#include "tilemapPL.h"
#include <glm/glm.hpp>
#include "typedefs.h"
#include "TileWorld.h"

extern TileWorld* tileWolrdGlobalRef;

extern glm::vec2 GcameraPos;

class Player;

namespace global {

	extern Player* player;

	extern Scene* mainScene;

	extern AssetManager* assetManager;
}