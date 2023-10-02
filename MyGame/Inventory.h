#pragma once

#include <stdint.h>
#include <string>

struct ItemDefinition
{
	uint32_t ID;
	std::string name;
	std::string description;
	int maxStack;
};

struct ItemInstanceStack
{
	uint32_t definitionID;
	int count;
	uint32_t dynmicIdentifier = 0;
};