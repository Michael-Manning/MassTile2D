#include "stdafx.h"

#include <memory>
#include <robin_hood.h>

#include "ItemIDs.h"

#include "Tool.h"

#include "Sword.h"
#include "Pickaxe.h"

const ToolClassFactoryMap ToolFactoryMap = {
	REG_TOOL(Sword),
	REG_TOOL(Pickaxe)
};
