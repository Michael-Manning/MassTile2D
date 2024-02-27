#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "typedefs.h"

//#include "robin_hood.h"

typedef uint32_t itemID;
constexpr itemID NULL_itemID = 0;
constexpr itemID MAX_itemID = 300;

class ItemBase;

class ItemLibrary {

public:

	ItemLibrary() {
		itemBaseLookup.resize(MAX_itemID);
	}

	void PopulateTools(std::string filepath);
	void PopulateConsumables(std::string filepath);
	//void PopulateBlocks(std::string csvData);

	const ItemBase& GetItem(itemID ID) const {
		return itemBaseLookup.at(ID);
	}

private:
	//robin_hood::unordered_flat_map<itemID, ItemBase> itemBaseLookup;
	std::vector<ItemBase> itemBaseLookup;

};

extern ItemLibrary itemLibrary;

struct ItemBase
{
	enum class Type {
		Empty = 0,
		Tool,
		Block,
		Consumable,
	};

	itemID ID = NULL_itemID;
	Type type = Type::Empty;
	std::string name = "";
	std::string description = "";
	int maxStack = 0;
	int inventorySpriteAtlasIndex = -1;
};

struct ItemStack
{
	itemID item = NULL_itemID;
	int count;
	uint32_t dynmicIdentifier = 0;

	bool InUse() const {
		return count > 0;
	}
	void Clear() {
		count = 0;
		item = NULL_itemID;
	}
};


struct InventoryContainer {

	InventoryContainer(int size) : size(size), slots(size) 
	{}

	const int size;
	std::vector<ItemStack> slots;
};

namespace Inventory {

	void MoveStack(InventoryContainer* srcContainer, int srcSlot, InventoryContainer* dstContainer, int dstSLot);
	void MoveMergeStack(InventoryContainer* srcContainer, int srcSlot, InventoryContainer* dstContainer, int dstSLot, int count);
}
