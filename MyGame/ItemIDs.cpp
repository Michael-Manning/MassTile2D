#include "stdafx.h"

#include <vector>

#include "Inventory.h"
#include "ItemIDs.h"


// auto gen in python

ItemLibrary::ItemLibrary() : itemBaseLookup{
	ItemBase(12, ItemBase::Type::Tool, "test", "test", 12, 12, 12)
} {}