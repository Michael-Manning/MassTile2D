#pragma once

#include <vector>
#include <string>
#include <memory>


#include "typedefs.h"

const auto Font_extension = ".font";

struct charQuad {
	glm::vec2 uvmin;
	glm::vec2 uvmax;
	glm::vec2 scale;
	glm::vec2 position;
};

class Font{

public:

	std::string name;

	int firstChar = 32;
	int charCount = 96;
	float fontHeight = 32;

	spriteID atlas;

	fontID ID;

	std::vector<charQuad> quads;

	void serializeJson(std::string filepath) {
	}

	static std::shared_ptr<Font> deserializeJson(std::string filepath) {
	}
	
};