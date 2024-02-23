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


	SpriteRenderer (const AssetPack::SpriteRenderer* s, Entity* entityCache) : _entityCache(entityCache){
		sprite = s->spriteID();
		atlasIndex = s->atlasIndex();
	};

	SpriteRenderer(spriteID sprite, int atlasIndex, Entity* entityCache, Sprite* spriteCache) :
		sprite(sprite),
		atlasIndex(atlasIndex),
		_entityCache(entityCache),
		_spriteCache(spriteCache)
	{}

	SpriteRenderer(const SpriteRenderer& data, Entity* entityCache, Sprite* spriteCache) :
		sprite(data.sprite),
		atlasIndex(data.atlasIndex),
		_entityCache(entityCache),
		_spriteCache(spriteCache)
	{}

	SpriteRenderer(const SpriteRenderer& data, Entity* entityCache) :
		sprite(data.sprite),
		atlasIndex(data.atlasIndex),
		_entityCache(entityCache)
	{}

	void SetSprite(Sprite* spr) {
		sprite = spr->ID;
		_spriteCache = spr;
	}

	Entity* _entityCache = nullptr;
	Sprite* _spriteCache = nullptr;
};
