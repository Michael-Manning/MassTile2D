#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <fstream>

#include "serialization.h"
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

	nlohmann::json serializeJson() {
		nlohmann::json j;
		j["uvmin"] = toJson(uvmin);
		j["uvmax"] = toJson(uvmax);
		j["scale"] = toJson(scale);
		j["xOff"] = xOff;
		j["yOff"] = yOff;
		j["advance"] = advance;
	}

	static packedChar deserializeJson(nlohmann::json& j) {
		packedChar pc;

		pc.uvmin = fromJson<glm::vec2>(j["uvmin"]);
		pc.uvmax = fromJson<glm::vec2>(j["uvmax"]);
		pc.scale = fromJson<glm::vec2>(j["scale"]);
		pc.xOff = j["xOff"];
		pc.yOff =  j["yOff"];
		pc.advance = j["advance"];

		return pc;
	}
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

		nlohmann::json ja;
		nlohmann::json j;

		j["name"] = name;
		j["firstChar"] = firstChar;
		j["charCount"] = charCount;
		j["fontHeight"] = fontHeight;
		j["atlas"] = atlas;
		j["ID"] = ID;

		for (auto& c : packedChars)
			j["packedChars"].push_back(c.serializeJson());

		for (auto& c : kerningTable)
			j["kerningTable"].push_back(c);

		ja["font"] = j;

		std::ofstream output(filepath);
		output << ja.dump(4) << std::endl;
		output.close();
	};

	static std::shared_ptr<Font> deserializeJson(std::string filepath) {

		std::ifstream input(filepath);
		nlohmann::json ja;
		input >> ja;
		nlohmann::json j = ja["font"];
		auto font = std::make_shared<Font>();

		font->name = j["font"];
		font->firstChar = j["firstChar"];
		font->charCount = j["charCount"];
		font->fontHeight = j["fontHeight"];
		font->atlas = j["atlas"];
		font->ID = j["ID"];

		for (auto& c : j["packedChars"])
			font->packedChars.push_back(packedChar::deserializeJson(c));

		for (auto& c : j["kerningTable"])
			font->kerningTable[static_cast<uint32_t>(c[0])] = static_cast<float>(c[1]);

		return font;
	};
	
};