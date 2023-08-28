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

#include "Engine.h"
#include "ECS.h"
#include "typedefs.h"
#include "Utils.h"

#include "ball.h"
//#include "player.h"



#define REG_BEHAVIOR(classname) {cHash(#classname), {#classname, []() -> std::shared_ptr<Entity> { return std::make_shared<classname>(); } } }


//typedef std::unordered_map<uint32_t, std::pair<std::string, std::function<std::shared_ptr<Entity>()>>> BehaviorMap_t;


extern std::unordered_map<uint32_t, std::pair<std::string, std::function<std::shared_ptr<Entity>()>>> BehaviorMap;

//static std::unordered_map<uint32_t, std::pair<std::string, std::function<std::shared_ptr<Entity>()>>> BehaviorMap = {
//	REG_BEHAVIOR(Ball),
//	REG_BEHAVIOR(Player),
//};

//{cHash("Ball"), []() -> std::shared_ptr<Entity> { return std::make_shared<Ball>(); } }