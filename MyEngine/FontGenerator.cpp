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

#include "vulkan_util.h"
#include "FontGenerator.h"
#include "Font.h"

using namespace std;
using namespace glm;

Font GenerateFontAtlas(std::string path, std::string exportPath, FontConfig& config, Engine& engine) {


	auto fontBuffer = VKUtil::readFile(path);
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

	{
		stbtt_pack_context fnt_context;

		STBTT_assert(stbtt_PackBegin(&fnt_context, bitmap.data(), config.atlasWidth, config.atlasHeight, 0, 0, NULL));

		stbtt_PackSetOversampling(&fnt_context, config.oversample, config.oversample);
		STBTT_assert(stbtt_PackFontRange(&fnt_context, reinterpret_cast<unsigned char*>(fontBuffer.data()), 0, config.fontHeight, config.firstChar, config.charCount, packedChars.data()));


		stbtt_PackEnd(&fnt_context);
	}

	// save atlas to disk
	stbi_flip_vertically_on_write(false);

	constexpr int channels = 1;
	if (!stbi_write_png(exportPath.c_str(), config.atlasWidth, config.atlasHeight, channels, bitmap.data(), config.atlasWidth * channels)) {
		throw std::exception("font atlas export error");
	}

	// create a sprite as well
	auto sprite = engine.assetManager->GenerateSprite(exportPath, FilterMode::Nearest);
	sprite->serializeJson(engine.assetManager->directories.assetDir + sprite->fileName + std::string(".sprite"));

	Font font;

	std::string name = std::filesystem::path(exportPath).filename().string();
	size_t lastindex = name.find_last_of(".");
	std::string rawname = name.substr(0, lastindex);

	font.name = name;
	font.firstChar = config.firstChar;
	font.charCount = config.charCount;
	font.fontHeight = config.fontHeight;
	font.atlas = sprite->ID;

	font.packedChars.clear();
	font.packedChars.reserve(config.charCount);

	vector<stbtt_kerningentry> kerningTable(config.charCount * config.charCount);
	stbtt_GetKerningTable(&font_info, kerningTable.data(), config.charCount * config.charCount);

	int ch = config.firstChar;
	for (auto& c : packedChars) {

		char letter = (char)ch;

		int ax;
		int lsb;
		stbtt_GetCodepointHMetrics(&font_info, ch, &ax, &lsb);

		ch++;

		float axf = ax * SF;

		packedChar pc;
		vec2 pixelsMin = glm::vec2(c.x0, c.y0);
		vec2 pixelsMax = glm::vec2(c.x1, c.y1);
		pc.uvmin = pixelsMin / vec2(config.atlasWidth, config.atlasHeight);
		pc.uvmax = pixelsMax / vec2(config.atlasWidth, config.atlasHeight);

		vec2 pixelScale = (pixelsMax - pixelsMin);
		pc.scale = pixelScale * pixelPositionScale / 2.0f;


		//pc.xOff = 0;
		pc.xOff = c.xoff * pixelPositionScale;
		pc.yOff = c.yoff * pixelPositionScale;

		pc.advance = c.xadvance * pixelPositionScale * 2.0f;

		font.packedChars.push_back(pc);
	}

	font.kerningTable.clear();

	for (size_t i = 0; i < font.charCount; i++)
	{
		int g1 = stbtt_FindGlyphIndex(&font_info, font.firstChar + i);
		for (size_t j = 0; j < font.charCount; j++)
		{
			int g2 = stbtt_FindGlyphIndex(&font_info, font.firstChar + j);
			int advance = stbtt_GetGlyphKernAdvance(&font_info, g1, g2);
			uint32_t hash = font.kernHash(i + font.firstChar, j + font.firstChar);
			if (i + font.firstChar == 'A' && j + font.firstChar == 'V') {
				int tset = 3;
			}
			font.kerningTable[hash] = 0;
			//font.kerningTable[hash] = advance * SF * UV_PixelScale * 2.0f;
			font.kerningTable[hash] = advance * SF * pixelPositionScale * 2.0f;

		}
	}

	return font;
}



/*




stbtt_packedchar* char_data; // This should be the array you passed to stbtt_PackFontRange
stbtt_fontinfo font;
const char* text = "Your string here";
int length = strlen(text);

// Retrieve font metrics for scaling
int ascent, descent, lineGap;
stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
float scaleY = fontSize / (ascent - descent);

letterDrawDetail* details = new letterDrawDetail[length];
float cursorX = 0;  // Starts at the beginning of your line

for (int i = 0; i < length; ++i) {
	int index = text[i] - 32; // Assuming you started with ASCII 32 (' ')

	details[i].x0 = char_data[index].x0;
	details[i].y0 = char_data[index].y0;
	details[i].x1 = char_data[index].x1;
	details[i].y1 = char_data[index].y1;

	details[i].positionX = cursorX + char_data[index].xoff;
	details[i].positionY = char_data[index].yoff + ascent * scaleY;

	details[i].scaleX = char_data[index].x1 - char_data[index].x0;
	details[i].scaleY = char_data[index].y1 - char_data[index].y0;

	cursorX += char_data[index].xadvance;
}



int prev_char = -1;  // Initialize with an invalid character

for (int i = 0; i < length; ++i) {
	int curr_char = text[i];
	int index = curr_char - 32; // Assuming you started with ASCII 32 (' ')

	if (prev_char != -1) {
		cursorX += stbtt_GetCodepointKernAdvance(&font, prev_char, curr_char);
	}

	details[i].x0 = char_data[index].x0;
	details[i].y0 = char_data[index].y0;
	details[i].x1 = char_data[index].x1;
	details[i].y1 = char_data[index].y1;

	details[i].positionX = cursorX + char_data[index].xoff;
	details[i].positionY = char_data[index].yoff + ascent * scaleY;

	details[i].scaleX = char_data[index].x1 - char_data[index].x0;
	details[i].scaleY = char_data[index].y1 - char_data[index].y0;

	cursorX += char_data[index].xadvance;

	prev_char = curr_char; // Set the current character as the previous one for the next iteration
}














*/