#pragma once

#include <vector>
#include <stdint.h>

#include <nlohmann/json.hpp>

#include "typedefs.h"
#include "Component.h"

#include <assetPack/common_generated.h>

class SpriteRenderer : public Component {
public:

	SpriteRenderer() {};

	SpriteRenderer(spriteID spr) {
		sprite = spr;
	};

	nlohmann::json serializeJson(entityID entId) const override;

	static SpriteRenderer deserializeJson(nlohmann::json j);

	spriteID sprite;

	SpriteRenderer duplicate() const {
		SpriteRenderer r(sprite);
		return r;
	};

	int atlasIndex = 0;

	static SpriteRenderer deserializeFlatbuffers(const AssetPack::SpriteRenderer* s) {
		SpriteRenderer r;
		r.sprite = s->spriteID();
		r.atlasIndex = s->atlasIndex();
		return r;
	};
};
