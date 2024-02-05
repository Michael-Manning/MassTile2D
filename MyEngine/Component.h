#pragma once

#include <nlohmann/json.hpp>
#include "typedefs.h"

class Component {
public:

	virtual nlohmann::json serializeJson(entityID entId) = 0;
};

// could move this somewhere else

struct ComponentResourceToken {
	int index = 0;
	bool active = false;
};