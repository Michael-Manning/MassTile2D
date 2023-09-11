#pragma once

#include <vector>
#include <stdint.h>

#include <nlohmann/json.hpp>

#include "typedefs.h"
#include "Component.h"

class TextRenderer : public Component {
public:

	TextRenderer() {};

	TextRenderer(spriteID spr) {
		sprite = spr;
	};

	nlohmann::json serializeJson(entityID entId) override;

	static TextRenderer deserializeJson(nlohmann::json j);

	spriteID sprite;

	TextRenderer duplicate() const {
		TextRenderer r(sprite);
		return r;
	};

	int atlasIndex = 0;
};
