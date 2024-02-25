#include "stdafx.h"

#include <memory>

#include "tilemapPL.h"
#include <glm/glm.hpp>
#include "typedefs.h"
#include "TileWorld.h"
#include "Player.h"


namespace global {

	Player* player = nullptr;
	Scene* mainScene;
	AssetManager* assetManager;
}