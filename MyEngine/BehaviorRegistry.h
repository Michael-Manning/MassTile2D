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
static std::unique_ptr<Behaviour> factory_ptr(classHash h, Entity* e)
{
	return std::make_unique<T>(h, e);
}

using BehaviourFactoryFunc = std::unique_ptr<Behaviour>(*)(classHash, Entity*);


#define REG_DERIVED(classname) {cHash(#classname), {#classname, &factory_ptr<classname> }}

using DerivedClassFactoryMap = robin_hood::unordered_flat_map<classHash, std::pair<std::string, BehaviourFactoryFunc>>;

const extern DerivedClassFactoryMap BehaviorMap;

