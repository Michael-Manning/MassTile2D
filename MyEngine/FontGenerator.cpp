#include "stdafx.h"

#include <string>
#include <stdint.h>
#include <vector>
#include <filesystem>
#include <stdint.h>

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif 

#include <stb_truetype.h>
#include <stb_image_write.h>

#include "Utils.h"
#include "FontGenerator.h"
#include "Font.h"

using namespace std;
using namespace glm;

Font GenerateFont_unidentified(const std::string& truetypeFilePath, std::string imageAtlasExportFileName, FontConfig& config) {

	auto fontBuffer = readFile(truetypeFilePath);
	vector<uint8_t> bitmap(config.atlasWidth * config.atlasHeight);

	vector<stbtt_packedchar> packedChars(config.charCount);
	stbtt_fontinfo font_info;

	if (!stbtt_InitFont(&font_info, reinterpret_cast<unsigned char*>(fontBuffer.data()), 0)) {
		throw std::exception("stb init error");
	}

	float SF = stbtt_ScaleForPixelHeight(&font_info, config.fontHeight);
	float pixelPositionScale = 1.0f / config.fontHeight;

	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &lineGap);

	float baseline;
	{
		int x0, x1, y0, y1;
		stbtt_GetFontBoundingBox(&font_info, &x0, &x1, &y0, &y1);
		baseline = SF * -y0 * pixelPositionScale;
	}

	// parse font file and generate bitmap data
	{
		stbtt_pack_context fnt_context;
		STBTT_assert(stbtt_PackBegin(&fnt_context, bitmap.data(), config.atlasWidth, config.atlasHeight, 0, 0, NULL));
		stbtt_PackSetOversampling(&fnt_context, config.oversample, config.oversample);
		STBTT_assert(stbtt_PackFontRange(&fnt_context, reinterpret_cast<unsigned char*>(fontBuffer.data()), 0, config.fontHeight, config.firstChar, config.charCount, packedChars.data()));
		stbtt_PackEnd(&fnt_context);
	}

	// save font atlas to disk
	stbi_flip_vertically_on_write(false);
	string imagePath = imageAtlasExportFileName;

	// add png extension if not present
	if (imagePath.substr(imagePath.size() - 4) != ".png") {
		imagePath += ".png";
	}

	constexpr int channels = 1;
	if (!stbi_write_png(imagePath.c_str(), config.atlasWidth, config.atlasHeight, channels, bitmap.data(), config.atlasWidth * channels)) {
		throw std::exception("font atlas export error");
	}

	// convert all required stb_truetype metadata in the the font structure used by the engine

	Font font;

	std::string name = std::filesystem::path(imagePath).filename().string();
	size_t lastindex = name.find_last_of(".");
	std::string rawname = name.substr(0, lastindex);

	font.name = rawname;
	font.firstChar = config.firstChar;
	font.charCount = config.charCount;
	font.fontHeight = config.fontHeight;
	font.lineGap = (float)(ascent - descent + lineGap) * SF * pixelPositionScale;
	//font.baseline = baseline;
	font.baseline = ascent * SF * pixelPositionScale;
	//font.atlas = sprite->ID;


	font.packedChars.clear();
	font.packedChars.reserve(config.charCount);

	vector<stbtt_kerningentry> kerningTable(config.charCount * config.charCount);
	stbtt_GetKerningTable(&font_info, kerningTable.data(), config.charCount * config.charCount);

	for (auto& c : packedChars) {

		packedChar pc;
		vec2 pixelsMin = glm::vec2(c.x0, c.y0);
		vec2 pixelsMax = glm::vec2(c.x1, c.y1);
		pc.uvmin = pixelsMin / vec2(config.atlasWidth, config.atlasHeight);
		pc.uvmax = pixelsMax / vec2(config.atlasWidth, config.atlasHeight);

		vec2 pixelScale = (pixelsMax - pixelsMin) / (float)config.oversample;
		pc.scale = pixelScale * pixelPositionScale;

		pc.xOff = c.xoff * pixelPositionScale + pc.scale.x / 2;
		pc.yOff = -c.yoff * pixelPositionScale - pc.scale.y / 2;

		pc.advance = c.xadvance * pixelPositionScale;

		font.packedChars.push_back(pc);
	}

	// create hash map kerning table 
	font.kerningTable.resize(font.charCount * font.charCount);
	for (size_t i = 0; i < font.charCount; i++)
	{
		int g1 = stbtt_FindGlyphIndex(&font_info, font.firstChar + i);
		for (size_t j = 0; j < font.charCount; j++)
		{
			int g2 = stbtt_FindGlyphIndex(&font_info, font.firstChar + j);
			int advance = stbtt_GetGlyphKernAdvance(&font_info, g1, g2);
			uint32_t hash = font.kernHash(i + font.firstChar, j + font.firstChar);
			font.kerningTable[hash] = advance * SF * pixelPositionScale;
		}
	}

	return font;
}