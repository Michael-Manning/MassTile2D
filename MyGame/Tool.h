#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <robin_hood.h>

#include "ItemIDs.h"


class Tool {
public:
	struct PlayerCtx {
		glm::vec2 position;
		float facingX;
	};

	virtual void Update(const PlayerCtx& ctx) = 0;
};


template<typename T>
static std::unique_ptr<Tool> tool_factory_ptr()
{
	return std::make_unique<T>();
}

using ToolFactoryFunc = std::unique_ptr<Tool>(*)();


#define REG_TOOL(classname) {classname##_ItemID, &tool_factory_ptr<classname>}

using ToolClassFactoryMap = robin_hood::unordered_flat_map<itemID, ToolFactoryFunc>;

const extern ToolClassFactoryMap ToolFactoryMap;
