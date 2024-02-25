#pragma once

#include <stdint.h>
#include <string>

typedef itemID;

struct ItemBase
{
	uint32_t ID;
	std::string name;
	std::string description;
	int maxStack;
};

struct ItemInstanceStack
{
	itemID item;
	int count;
	uint32_t dynmicIdentifier = 0;
};