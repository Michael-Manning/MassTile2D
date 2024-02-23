#include "stdafx.h"

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

#include "Font.h"


void Font::serializeJson(std::string filepath) const{

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


	std::ofstream output(filepath);
	output << j.dump(3) << std::endl;
	output.close();
};
void Font::serializeBinary(std::string filepath) const{
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
void Font::deserializeBinary(std::string filepath, Font* font) {

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

	return;
}
Font::Font(const AssetPack::Font* f) {

	name = f->name()->str();
	firstChar = f->firstChar();
	charCount = f->charCount();
	fontHeight = f->fontHeight();
	baseline = f->baseline();
	lineGap = f->lineGap();
	atlas = f->atlas();
	ID = f->ID();

	packedChars.resize(f->packedChars()->size());
	for (size_t i = 0; i < packedChars.size(); i++)
		packedChars[i] = packedChar::deserializeFlatbuffer(f->packedChars()->Get(i));

	kerningTable.resize(f->kerningTable()->size());
	for (size_t i = 0; i < kerningTable.size(); i++)
		kerningTable[i] = f->kerningTable()->Get(i);

	return;
}


void CalculateQuads(Font* f, std::string& text, charQuad* quads) {

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
		char c2 = text[i + 1];
		if (c2 != '\0')
			cursor.x += f->kerningTable[f->kernHash(c, c2)];
		quads[i] = q;
	}
};