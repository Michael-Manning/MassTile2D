#include <string>
#include <stdint.h>
#include <vector>
#include <filesystem>

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

Font GenerateFontAtlas(std::string path, std::string exportPath, FontConfig& config, Engine& engine) {

	
	auto fontBuffer = VKUtil::readFile(path);
	vector<uint8_t> bitmap(config.atlasWidth * config.atlasHeight);

	vector<stbtt_packedchar> packedChars(config.charCount);
	stbtt_fontinfo font_info;

	if (!stbtt_InitFont(&font_info, reinterpret_cast<unsigned char*>(fontBuffer.data()), 0)) {
		throw std::exception("stb init error");
	}

	float scale = stbtt_ScaleForPixelHeight(&font_info, config.fontHeight);

	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &lineGap);

	//ascent = roundf(ascent * scale);
	//descent = roundf(descent * scale);

	{
		stbtt_pack_context fnt_context;

		STBTT_assert(stbtt_PackBegin(&fnt_context, bitmap.data(), config.atlasWidth, config.atlasHeight, 0, 0, NULL));

		stbtt_PackSetOversampling(&fnt_context, config.oversample, config.oversample);
		STBTT_assert(stbtt_PackFontRange(&fnt_context, reinterpret_cast<unsigned char*>(fontBuffer.data()), 0, 32, config.firstChar, config.charCount, packedChars.data()));

		
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

	for (auto& c : packedChars) {
		packedChar pc;
		pc.uvmin = glm::vec2(c.x0, c.y0) / glm::vec2(config.atlasWidth, config.atlasHeight);
		pc.uvmax = glm::vec2(c.x1, c.y1) / glm::vec2(config.atlasWidth, config.atlasHeight);
		pc.scale = glm::vec2(pc.uvmax - pc.uvmin) / glm::vec2(config.atlasWidth, config.atlasHeight);
		pc.xOff = c.xoff * scale;
		pc.yOff = c.yoff * scale;
		font.packedChars.push_back(pc);
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