#pragma once

#include <vector>
#include <string>
#include <memory>


#include "typedefs.h"

const auto Font_extension = ".font";

struct charQuad {
	alignas(8) glm::vec2 uvmin;
	alignas(8) glm::vec2 uvmax;
	alignas(8) glm::vec2 scale;
	alignas(8) glm::vec2 position;
};
static_assert(sizeof(charQuad) % 16 == 0);

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
		auto font = std::make_shared<Font>();
		return font;
	}
	
};