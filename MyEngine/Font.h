#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>


#include "typedefs.h"

const auto Font_extension = ".font";

struct charQuad {
	alignas(8) glm::vec2 uvmin;
	alignas(8) glm::vec2 uvmax;
	alignas(8) glm::vec2 scale;
	alignas(8) glm::vec2 position;
};
static_assert(sizeof(charQuad) % 16 == 0);

struct packedChar {
	glm::vec2 uvmin;
	glm::vec2 uvmax;
	glm::vec2 scale;
	float xOff, yOff, advance;
};

class Font{

public:


	std::string name;

	int firstChar = 32;
	int charCount = 96;
	float fontHeight = 32;

	spriteID atlas;

	fontID ID;

	// TODO: enforce font count < 128, then just use bit shifting to do this
	inline uint32_t kernHash(char a, char b) {
		return (a - firstChar) * charCount + (b - firstChar);
	};
	std::unordered_map<uint32_t, float> kerningTable;

	std::vector<packedChar> packedChars;

	packedChar operator [] (char c) const { return packedChars[c - firstChar]; }

	void serializeJson(std::string filepath) {
	}

	static std::shared_ptr<Font> deserializeJson(std::string filepath) {
		auto font = std::make_shared<Font>();
		return font;
	}
	
};