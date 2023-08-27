#pragma once

#include <vector>
#include <stdint.h>

#include <nlohmann/json.hpp>

#include "typedefs.h"
#include "Component.h"

class SpriteRenderer : public Component {
public:

	SpriteRenderer() {};

	SpriteRenderer(spriteID spr) {
		sprite = spr;
	};

	nlohmann::json serializeJson(entityID entId) override;

	static SpriteRenderer deserializeJson(nlohmann::json j);

	spriteID sprite;

	SpriteRenderer duplicate() const {
		SpriteRenderer r(sprite);
		return r;
	};

	int atlasIndex = 0;
};
