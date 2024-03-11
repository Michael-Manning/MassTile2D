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

	// data
	spriteID sprite;
	int atlasIndex = 0;
	bool useLightMap = false;

	nlohmann::json serializeJson(entityID entId) const override {
		nlohmann::json j;
		j["entityID"] = entId;
		j["spriteID"] = sprite;
		j["atlasIndex"] = atlasIndex;
		j["useLightMap"] = useLightMap;
		return j;
	}

	SpriteRenderer(nlohmann::json j) :
		sprite(j["spriteID"].get<uint32_t>()),
		atlasIndex(j["atlasIndex"]),
		useLightMap(j.contains("useLightMap") ? (bool)j["useLightMap"] : false)
	{}

	SpriteRenderer(const AssetPack::SpriteRenderer* s, Entity* entityCache) : _entityCache(entityCache) {
		sprite = s->spriteID();
		atlasIndex = s->atlasIndex();
		useLightMap = s->useLightMap();
	};

	SpriteRenderer(spriteID sprite, int atlasIndex, Entity* entityCache, Sprite* spriteCache) :
		sprite(sprite),
		atlasIndex(atlasIndex),
		useLightMap(useLightMap),
		_entityCache(entityCache),
		_spriteCache(spriteCache)
	{}

	SpriteRenderer(const SpriteRenderer& data, Entity* entityCache, Sprite* spriteCache) :
		sprite(data.sprite),
		atlasIndex(data.atlasIndex),
		useLightMap(data.useLightMap),
		_entityCache(entityCache),
		_spriteCache(spriteCache)
	{}

	SpriteRenderer(const SpriteRenderer& data, Entity* entityCache) :
		sprite(data.sprite),
		atlasIndex(data.atlasIndex),
		useLightMap(data.useLightMap),
		_entityCache(entityCache)
	{}

	void SetSprite(Sprite* spr) {
		sprite = spr->ID;
		_spriteCache = spr;
	}

	Entity* _entityCache = nullptr;
	Sprite* _spriteCache = nullptr;
};
