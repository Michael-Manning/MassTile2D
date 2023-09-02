#pragma once

#include <memory>

#include "tilemapPL.h"
#include <glm/glm.hpp>
#include "typedefs.h"
#include "TileWorld.h"

extern std::shared_ptr<TileWorld> tileWolrdGlobalRef;

extern glm::vec2 GcameraPos;


namespace Tiles {
	constexpr blockID Grass = 0;
	constexpr blockID Dirt = 1;
	constexpr blockID Stone = 2;
	constexpr blockID Iron = 3;
	constexpr blockID Air = 1023;
}