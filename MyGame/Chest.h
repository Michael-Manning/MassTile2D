#pragma once

#include <glm/glm.hpp>

#include "MapEntity.h"
#include "Behaviour.h"

#include "Inventory.h"

#include <assetPack/SceneEntities_generated.h>


class Chest : public MapEntity {

public:

	InventoryContainer container;

	Chest(const AssetPack::Chest* chest, const AssetPack::MapEntityBase * mapBase) : MapEntity(mapBase), container(chest->container()) {
		
	}

};
