#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

#include "serialization.h"
#include "BinaryWriter.h"
#include "typedefs.h"

#include <assetPack/Font_generated.h>

const auto Font_extension = ".font";

// Serialize using binary during debug for speed. (takes 600ms per font using json)
// Serialize using json on creation for asset packager to automatically convert to flatbuffer
// serialize serialize flatbuffer during published build with packed assets

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

	nlohmann::json serializeJson() const {
		nlohmann::json j;
		j["uvmin"] = toJson(uvmin);
		j["uvmax"] = toJson(uvmax);
		j["scale"] = toJson(scale);
		j["xOff"] = xOff;
		j["yOff"] = yOff;
		j["xAdvance"] = advance;

		return j;
	};

	static packedChar deserializeJson(nlohmann::json& j) {
		packedChar pc;

		pc.uvmin = fromJson<glm::vec2>(j["uvmin"]);
		pc.uvmax = fromJson<glm::vec2>(j["uvmax"]);
		pc.scale = fromJson<glm::vec2>(j["scale"]);
		pc.xOff = j["xOff"];
		pc.yOff = j["yOff"];
		pc.advance = j["advance"];

		return pc;
	};

	static packedChar deserializeFlatbuffer(const AssetPack::packedChar* p) {
		packedChar pc;
		
		pc.uvmin = fromAP(p->uvmin());
		pc.uvmax = fromAP(p->uvmax());
		pc.scale = fromAP(p->scale());
		pc.xOff = p->xOff();
		pc.yOff = p->yOff();
		pc.advance = p->xAdvance();

		return pc;
	};
};

class Font {

public:

	Font() {}

	std::string name;

	int firstChar = 32;
	int charCount = 96;
	float fontHeight = 32;

	float baseline;
	float lineGap;

	spriteID atlas;

	fontID ID;

	// TODO: enforce font count < 128, then just use bit shifting to do this
	inline uint32_t kernHash(char a, char b) {
		return (a - firstChar) * charCount + (b - firstChar);
	};
	std::vector<float> kerningTable;

	std::vector<packedChar> packedChars;

	packedChar operator [] (char c) const { return packedChars[c - firstChar]; }

	void serializeJson(std::string filepath) const;
	void serializeBinary(std::string filepath) const;
	static void deserializeBinary(std::string filepath, Font* font);
	Font(const AssetPack::Font* f);
};

void CalculateQuads(Font* f, std::string& text, charQuad* quads);