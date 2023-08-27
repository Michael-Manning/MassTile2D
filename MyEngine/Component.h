#pragma once

#include <nlohmann/json.hpp>
#include "typedefs.h"

class Component {
public:

	virtual nlohmann::json serializeJson(entityID entId) = 0;
};
