#pragma once

#include <string>

#include "Engine.h"

struct  FontConfig{
	int firstChar = 32;
	int charCount = 96;
	float fontHeight = 32;
	int atlasWidth = 512;
	int atlasHeight = 512;
	int oversample = 4;
};



void GenerateFontAtlas(std::string fontPath, std::string exportPath, FontConfig & config, Engine& engine);