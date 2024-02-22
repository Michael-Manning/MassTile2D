#pragma once

#include <vector>
#include <stdint.h>

#include <nlohmann/json.hpp>

#include "typedefs.h"
#include "Component.h"
#include "Entity.h"

#include <assetPack/common_generated.h>

class SpriteRenderer : public Component {
public:

	SpriteRenderer() {};

	SpriteRenderer(spriteID spr) {
		sprite = spr;
	};

	nlohmann::json serializeJson(entityID entId) const override;

	static SpriteRenderer deserializeJson(nlohmann::json j);

	// data
	spriteID sprite;
	int atlasIndex = 0;

	//SpriteRenderer duplicate() const {
	//	SpriteRenderer r(sprite);
	//	return r;
	//};


	static SpriteRenderer deserializeFlatbuffers(const AssetPack::SpriteRenderer* s) {
		SpriteRenderer r;
		r.sprite = s->spriteID();
		r.atlasIndex = s->atlasIndex();
		return r;
	};

	SpriteRenderer(spriteID sprite, int atlasIndex, Entity* entityCache) :
		sprite(sprite),
		atlasIndex(atlasIndex),
		_entityCache(entityCache)
	{}

	SpriteRenderer(const SpriteRenderer& data, Entity* entityCache) :
		sprite(data.sprite),
		atlasIndex(data.atlasIndex),
		_entityCache(entityCache)
	{}

	Entity* _entityCache = nullptr;
};
