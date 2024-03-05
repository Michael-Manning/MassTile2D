#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include <robin_hood.h>

#include <assetPack/SceneEntities_generated.h>

#include "typedefs.h"
#include "ItemIDs.h"

//#include "robin_hood.h"

constexpr itemID NULL_itemID = 0;
constexpr itemID MAX_itemID = 300;

struct ItemBase
{
	enum class Type {
		Empty = 0,
		Tool,
		Block,
		Consumable,
		MapEntity
	};

	const itemID ID = NULL_itemID;
	const Type type = Type::Empty;
	const std::string name = "";
	const std::string description = "";
	const int maxStack = 0;
	const spriteID sprite = 0;
	const int atlasIndex = 0;

	ItemBase() {}; // allow empty item entries

	ItemBase(itemID ID, Type type, std::string name, std::string description, int maxStack, spriteID sprite, int atlasIndex) :
		ID(ID), type(type), name(name), description(description), maxStack(maxStack), sprite(sprite), atlasIndex(atlasIndex) {}
};


class ItemLibrary {

public:

	/*ItemLibrary() {
		itemBaseLookup.resize(MAX_itemID);
	}*/

	ItemLibrary();

	void PopulateTools(std::string filepath);
	void PopulateConsumables(std::string filepath);
	void PopulateBlocks(std::string filepath);

	const ItemBase& GetItem(itemID ID) const {
		return itemBaseLookup.at(ID);
	}

	blockID GetBlock(itemID ID) const {
		return blockLookup.at(ID);
	}
	itemID GetBlockItemID(blockID ID) const {
		return blockItemLookup.at(ID);
	}

private:
	//robin_hood::unordered_flat_map<itemID, ItemBase> itemBaseLookup;
	const std::vector<ItemBase> itemBaseLookup;

	robin_hood::unordered_flat_map<itemID, blockID> blockLookup;
	robin_hood::unordered_flat_map<blockID, itemID> blockItemLookup;

};

extern ItemLibrary itemLibrary;

struct ItemStack
{
	itemID item = NULL_itemID;
	int count = 0;
	uint32_t dynmicIdentifier = 0;

	bool InUse() const {
		return count > 0;
	}
	void Clear() {
		count = 0;
		item = NULL_itemID;
	}

	ItemStack() {}
	ItemStack(itemID item, int count) : item(item), count(count) {}

	ItemStack(const AssetPack::ItemStack* itemStack) : item(itemStack->item()), count(itemStack->count()), dynmicIdentifier(itemStack->dynmicIdentifier())
	{}

	AssetPack::ItemStack  Serialize() const {
		return AssetPack::ItemStack(item, count, dynmicIdentifier);
	}
};

template<typename T, typename PackT>
static void deserializeVector(std::vector<T>& vec, PackT getf) {
	vec.resize(getf->size());
	for (size_t i = 0; i < vec.size(); i++)
		vec[i] = T(getf->Get(i));
}


struct InventoryContainer {

	InventoryContainer(int size) : size(size), slots(size)
	{}

	const int size;
	std::vector<ItemStack> slots;

	InventoryContainer(const AssetPack::InventoryContainer* container) : size(container->size()) {
		deserializeVector(slots, container->slots());
		//slots.resize(container->slots()->size());
		//for (size_t i = 0; i < slots.size(); i++) 
		//	slots[i] = ItemStack(container->slots()->Get(i));	
	}

	auto Serialize(flatbuffers::FlatBufferBuilder& builder) const {

		std::vector<AssetPack::ItemStack> stackVec;
		for (auto& stack : slots)
			stackVec.push_back(stack.Serialize());

		return AssetPack::CreateInventoryContainer(builder,
			size,
			builder.CreateVectorOfStructs(stackVec.data(), stackVec.size())
		);

		//auto pack = AssetPack::InventoryContainerBuilder(builder);
		//pack.add_size(size);	
		//pack.add_slots(builder.CreateVectorOfStructs(stackVec.data(), stackVec.size()));
		//return pack.Finish();
	}
};

namespace Inventory {

	void MoveStack(InventoryContainer* srcContainer, int srcSlot, InventoryContainer* dstContainer, int dstSLot);
	void MoveMergeStack(InventoryContainer* srcContainer, int srcSlot, InventoryContainer* dstContainer, int dstSLot, int count);
}
