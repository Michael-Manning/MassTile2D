#pragma once

#include <string>
#include <stdint.h>
#include <utility>
#include <memory>

#include <robin_hood.h>

#include "Behaviour.h"
#include "typedefs.h"
#include "Utils.h"


template<typename T>
static std::unique_ptr<Behaviour> factory_ptr(behavioiurHash h, Entity* e)
{
	return std::make_unique<T>(h, e);
}

using BehaviourFactoryFunc = std::unique_ptr<Behaviour>(*)(behavioiurHash, Entity*);


#define REG_BEHAVIOR(classname) {cHash(#classname), {#classname, &factory_ptr<classname> }}

using BehaviourFactoryMap = robin_hood::unordered_flat_map<behavioiurHash, std::pair<std::string, BehaviourFactoryFunc>>;

const extern BehaviourFactoryMap BehaviorMap;

