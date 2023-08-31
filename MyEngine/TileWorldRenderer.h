#pragma once

#include <vector>
#include <stdint.h>
#include <cassert>

#include <nlohmann/json.hpp>

#include "typedefs.h"
#include "Component.h"

class TileWorldRenderer : public Component {
public:

	TileWorldRenderer() {};

	TileWorldRenderer(spriteID spr) {
		sprite = spr;
	};

	nlohmann::json serializeJson(entityID entId) override;

	static TileWorldRenderer deserializeJson(nlohmann::json j);

	spriteID sprite;

	// only one instance allowed
	TileWorldRenderer duplicate() const {
		assert(false);
		return TileWorldRenderer();
	};
};
