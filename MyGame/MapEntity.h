#pragma once

#include <glm/glm.hpp>

#include <assetPack/SceneEntities_generated.h>
#include <assetPack/WorldData_generated.h>

#include "serialization.h"

class MapEntity {
public :

	// anchored bottom left tile
	glm::ivec2 position = glm::ivec2(0, 0);
	glm::ivec2 size = glm::ivec2(0, 0); // in tiles (does this actually need to be serialized? And if so, should that be manditory and not done in derived class instead if needed?)

	MapEntity(glm::ivec2 position, glm::ivec2 size) : position(position), size(size) {}

	MapEntity(const AssetPack::MapEntityBase* mapEntity) : position(fromAP(mapEntity->position())), size(fromAP(mapEntity->size())) {

	}

	MapEntity() {}

	// decide if I want this class to only support instantiating a prefab, or
	// if I want to support arbitrary entities as well to store extra state?

	// Actually I think this will be a base class to a different category of classes which
	// may include a prefab, but hold there own state which must be individually serialized

	//virtual void serialize(AssetPack::ChunkDataBuilder) = 0;

	auto SerializeBase() const {
		return AssetPack::MapEntityBase(toAP(position), toAP(size));
	}

	virtual void OnRightClick() {};
};