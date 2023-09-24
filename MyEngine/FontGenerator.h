#pragma once

#include <string>

#include "Font.h"

struct  FontConfig{
	int firstChar = 32;
	int charCount = 96;
	float fontHeight = 32;
	int atlasWidth = 1024;
	int atlasHeight = 1024;
	int oversample = 4;
};

Font GenerateFont_unidentified(const std::string& truetypeFilePath, std::string imageAtlasExportFileName, FontConfig & config);