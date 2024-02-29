#pragma once

#include <glm/glm.hpp>

#include "MapEntity.h"
#include "Behaviour.h"

#include "Inventory.h"

#include <assetPack/SceneEntities_generated.h>


class Chest : public MapEntity {
//class Chest {

private:
	const glm::ivec2 mapSize = glm::ivec2(2, 2);

	Chest(const Chest& other) = default;
	Chest& operator=(const Chest& other) = default;

public:

	Chest(Chest&& other) noexcept = default;

	InventoryContainer container;

	Chest(int containerSlots, glm::ivec2 anchor) : MapEntity(anchor, mapSize), container(containerSlots) {}

	Chest(const AssetPack::Chest* chest) : MapEntity(chest->MapeEntity()), container(chest->container()) {}

};
