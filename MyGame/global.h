#pragma once

#include "tilemapPL.h"
#include <glm/glm.hpp>
#include "typedefs.h"

// temporary until I deside how to make this data accessible through a tilemap renderer
extern TilemapPL* tileWolrdGlobalRef;

extern glm::vec2 GcameraPos;




namespace Tiles {
	constexpr blockID Grass = 0;
	constexpr blockID Dirt = 1;
	constexpr blockID Stone = 2;
	constexpr blockID Iron = 3;
	constexpr blockID Air = 1023;
}