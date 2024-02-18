#pragma once

#include <string>
#include <iostream>
#include <cstdint>
#include <concepts>
#include <stdint.h>
#include <unordered_map>
#include <utility>
//#include <functional>
#include <memory>

#include <robin_hood.h>

#include "Behaviour.h"
#include "typedefs.h"
#include "Utils.h"



// Constructs a compile time behaviour map entry, which is keyed by the name hash, and contains a name, behaviour copy function pair
//#define REG_BEHAVIOR(classname) {cHash(#classname), {#classname, [](ComponentAccessor* accessor, Entity* entityCache) -> std::unique_ptr<Behaviour> { return std::make_unique<classname>(cHash(#classname), accessor, entityCache); } } }
//
//
//const extern robin_hood::unordered_flat_map<behavioiurHash, std::pair<std::string, std::function<std::unique_ptr<Behaviour>(ComponentAccessor*, Entity*)>>> BehaviorMap;


 ///  CHASH CONSTANT EVALLLLLLLLLLLL DONT FORGETTTTTT

template<typename T>
static std::unique_ptr<Behaviour> factory_ptr(behavioiurHash h, ComponentAccessor* a, Entity* e)
{
	return std::make_unique<T>(h, a, e);
}

//template<typename T>
//static std::unique_ptr<Behaviour> factory_ptr(behavioiurHash h, ComponentAccessor* a)
//{
//	return std::make_unique<T>(h, a);
//}


using BehaviourFactoryFunc = std::unique_ptr<Behaviour>(*)(behavioiurHash, ComponentAccessor*, Entity*);

//#define REG_BEHAVIOR(classname) {cHash(#classname), {#classname, [](ComponentAccessor* accessor, Entity* entityCache) -> std::unique_ptr<Behaviour> { return std::make_unique<classname>(cHash(#classname), accessor, entityCache); } } }

#define REG_BEHAVIOR(classname) {cHash(#classname), {#classname, &factory_ptr<classname> }}

const extern robin_hood::unordered_flat_map<behavioiurHash, std::pair<std::string, BehaviourFactoryFunc>> BehaviorMap;






//#define REG_BEHAVIOR(classname) {cHash(#classname), {#classname, [](ComponentAccessor* accessor) -> std::unique_ptr<Behaviour> { return std::make_unique<classname>(cHash(#classname), accessor); } } }
//
//const std::unordered_map<behavioiurHash, std::function<std::unique_ptr<Behaviour>(ComponentAccessor*)>> BehaviorMap{
//	REG_BEHAVIOR(Player)
//};