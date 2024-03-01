#pragma once

#include <glm/glm.hpp>

#include "MapEntity.h"
#include "Behaviour.h"

#include "Inventory.h"
#include "global.h"

#include <assetPack/SceneEntities_generated.h>


class Chest : public MapEntity {

private:

	//static glm::ivec2 mapSize = glm::ivec2(2, 2);

	Chest(const Chest& other) = default;
	Chest& operator=(const Chest& other) = default;

	void placeSelf() {

		auto pos = global::tileWorld->TileWorldPos(position);

		global::mainScene->Instantiate(global::assetManager->GetPrefab("chest"), "", 
			global::tileWorld->TileWorldPos(position) + (glm::vec2(size / 2) * tileWorldSize));
	}

public:

	/*
	void OnDestry(){
		if(global::inspectedInventory == &container){
			global::inspectedInventory = nullptr;
		}
	}
	*/

	Chest(Chest&& other) noexcept = default;

	InventoryContainer container;

	Chest(int containerSlots, glm::ivec2 anchor) : MapEntity(anchor, glm::ivec2(2, 2)), container(containerSlots) {
		placeSelf();
	}

	Chest(const AssetPack::Chest* chest) : MapEntity(chest->MapeEntity()), container(chest->container()) {
		placeSelf();
	}

#include <iostream>
	void OnRightClick() override {
		//std::cout << "click" << std::endl;
		global::inspectedInventory = &container;
	}

};
