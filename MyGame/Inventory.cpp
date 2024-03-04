#include "stdafx.h"

#include <stdint.h>
#include <string>
#include "typedefs.h"

#include <fast-cpp-csv-parser/csv.h>

#include "robin_hood.h"

#include "Inventory.h"

ItemLibrary itemLibrary;

#if false
void ItemLibrary::PopulateTools(std::string filepath)
{
	io::CSVReader<6> in(filepath);
	in.read_header(io::ignore_extra_column, "holdSpriteAtlasIndex", "itemID", "name", "description", "maxStack", "inventorySpriteAtlasIndex");
	int holdSpriteAtlasIndex; itemID ID; std::string name; std::string description; int maxStack; int inventorySpriteAtlasIndex;
	while (in.read_row(holdSpriteAtlasIndex, ID, name, description, maxStack, inventorySpriteAtlasIndex)) {

		//auto myItem = 

		//itemBaseLookup[ID] = ItemBase{
		//	.ID = ID,
		//	.type = ItemBase::Type::Tool,
		//	.name = name,
		//	.description = description,
		//	.maxStack = maxStack,
		//	.inventorySpriteAtlasIndex = inventorySpriteAtlasIndex
		//};

		//itemBaseLookup.emplace(itemBaseLookup.begin() + ID, ItemBase{
		itemBaseLookup[ID] = ItemBase{
			.ID = ID,
			.type = ItemBase::Type::Tool,
			.name = name,
			.description = description,
			.maxStack = maxStack,
			.inventorySpriteAtlasIndex = inventorySpriteAtlasIndex
			};
	}
}

void ItemLibrary::PopulateConsumables(std::string filepath)
{
	io::CSVReader<5> in(filepath);
	in.read_header(io::ignore_extra_column, "itemID", "name", "description", "maxStack", "inventorySpriteAtlasIndex");
	itemID ID; std::string name; std::string description; int maxStack; int inventorySpriteAtlasIndex;
	while (in.read_row(ID, name, description, maxStack, inventorySpriteAtlasIndex)) {
		//itemBaseLookup.emplace(itemBaseLookup.begin() + ID, ItemBase{
		itemBaseLookup[ID] = ItemBase{
			.ID = ID,
			.type = ItemBase::Type::Consumable,
			.name = name,
			.description = description,
			.maxStack = maxStack,
			.inventorySpriteAtlasIndex = inventorySpriteAtlasIndex
			};
	}
}

void ItemLibrary::PopulateBlocks(std::string filepath) {
	io::CSVReader<5> in(filepath);
	in.read_header(io::ignore_extra_column, "itemID", "name", "description", "maxStack", "blockID");
	itemID ID; std::string name; std::string description; int maxStack; blockID block;
	while (in.read_row(ID, name, description, maxStack, block)) {
		//itemBaseLookup.emplace(itemBaseLookup.begin() + ID, ItemBase{
		itemBaseLookup[ID] = ItemBase{
			.ID = ID,
			.type = ItemBase::Type::Block,
			.name = name,
			.description = description,
			.maxStack = maxStack,
			.inventorySpriteAtlasIndex = static_cast<int>(GetFloatingTile(block))
			};
		blockLookup.insert({ ID, block });
		blockItemLookup.insert({ block, ID });
	}
}
#endif

namespace Inventory {

	void MoveStack(InventoryContainer* srcContainer, int srcSlot, InventoryContainer* dstContainer, int dstSLot) {
		assert(srcContainer->slots[srcSlot].InUse() == true);
		assert(dstContainer->slots[dstSLot].InUse() == false);

		dstContainer->slots[dstSLot].item = srcContainer->slots[srcSlot].item;
		dstContainer->slots[dstSLot].count = srcContainer->slots[srcSlot].count;

		srcContainer->slots[srcSlot].item = NULL_itemID;
		srcContainer->slots[srcSlot].count = 0;
	}

	void MoveMergeStack(InventoryContainer* srcContainer, int srcSlot, InventoryContainer* dstContainer, int dstSLot, int count) {
		assert(srcContainer->slots[srcSlot].InUse() == true);
		assert(srcContainer->slots[srcSlot].InUse() == true);
		assert(dstContainer->slots[dstSLot].item == srcContainer->slots[srcSlot].item || dstContainer->slots[dstSLot].InUse() == false);

		dstContainer->slots[dstSLot].item = srcContainer->slots[srcSlot].item;
		dstContainer->slots[dstSLot].count += count;

		srcContainer->slots[srcSlot].count -= count;
		if (srcContainer->slots[srcSlot].count == 0)
			srcContainer->slots[srcSlot].item = NULL_itemID;




#ifdef _DEBUG
		// vreify  if moving to none-empty stack, that the itemIDs are the same and the max stack is not exceeded. Maybe return actual items moved?
#endif



	}
}