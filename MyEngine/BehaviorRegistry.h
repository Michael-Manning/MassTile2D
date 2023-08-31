#pragma once

#include <string>
#include <iostream>
#include <cstdint>
#include <concepts>
#include <stdint.h>
#include <unordered_map>
#include <utility>
#include <functional>
#include <memory>

#include "ECS.h"
#include "typedefs.h"
#include "Utils.h"


#define REG_BEHAVIOR(classname) {cHash(#classname), {#classname, []() -> std::shared_ptr<Entity> { return std::make_shared<classname>(); } } }


extern std::unordered_map<uint32_t, std::pair<std::string, std::function<std::shared_ptr<Entity>()>>> BehaviorMap;

