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

	void serializeBinary(std::string filepath) {
		BinaryWriter writer(filepath);

		writer << name;
		writer << firstChar;
		writer << charCount;
		writer << fontHeight;
		writer << baseline;
		writer << lineGap;
		writer << atlas;
		writer << ID;
		writer << packedChars;
		writer << kerningTable;
	}
	static std::shared_ptr<Font> deserializeBinary(std::string filepath) {
		auto font = std::make_shared<Font>();

		BinaryReader reader(filepath);
		reader >> font->name;
		reader >> font->firstChar;
		reader >> font->charCount;
		reader >> font->fontHeight;
		reader >> font->baseline;
		reader >> font->lineGap;
		reader >> font->atlas;
		reader >> font->ID;
		reader >> font->packedChars;
		reader >> font->kerningTable;

		return font;
	}
	static std::shared_ptr<Font> deserializeFlatbuffer(const AssetPack::Font* f) {
		auto font = std::make_shared<Font>();

		font->name = f->name()->str();
		font->firstChar = f->firstChar();
		font->charCount = f->charCount();
		font->fontHeight = f->fontHeight();
		font->baseline = f->baseline();
		font->lineGap = f->lineGap();
		font->atlas = f->atlas();
		font->ID = f->ID();
		
		font->packedChars.resize(f->packedChars()->size());
		for (size_t i = 0; i < font->packedChars.size(); i++)
			font->packedChars[i] = packedChar::deserializeFlatbuffer(f->packedChars()->Get(i));

		font->kerningTable.resize(f->kerningTable()->size());
		for (size_t i = 0; i < font->kerningTable.size(); i++)
			font->kerningTable[i] = f->kerningTable()->Get(i);

		return font;
	}
};

static void CalculateQuads(std::shared_ptr<Font> f, std::string& text, charQuad* quads) {

	glm::vec2 cursor = glm::vec2(0.0f);
	for (int i = 0; i < text.length(); i++) {
		char c = text[i];

		if (c == '\n') {
			cursor.x = 0.0f;
			cursor.y -= f->lineGap;
			continue;
		}

		auto packed = f->operator[](c);
		charQuad q;
		q.uvmax = packed.uvmax;
		q.uvmin = packed.uvmin;
		q.scale = packed.scale;
		q.position = glm::vec2(cursor.x + packed.xOff, cursor.y + packed.yOff - f->baseline);
		cursor.x += packed.advance;
		cursor.x += f->kerningTable[f->kernHash(c, text[i + 1])];
		quads[i] = q;
	}
};