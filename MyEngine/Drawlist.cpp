#include "stdafx.h"

#include <vector>

#include <glm/glm.hpp>

#include "pipelines.h"

#include "Drawlist.h"




inline void Drawlist::AddCenteredQuad(glm::vec4 color, glm::vec2 pos, glm::vec2 scale, float rotation) {

	assert(coloredQuadInstanceIndex < allocationSettings.ColoredQuad_MaxInstances);

	ColoredQuadPL::InstanceBufferData& item = coloredQuadInstanceData[coloredQuadInstanceIndex % allocationSettings.ColoredQuad_MaxInstances];
	item.color = color;
	item.position = pos;
	item.scale = scale;
	item.rotation = rotation;
	item.circle = 0;

	coloredQuadInstanceIndex++;
}

inline void Drawlist::AddCenteredSprite(Sprite* sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {

	assert(texturedQuadInstanceIndex < allocationSettings.TexturedQuad_MaxInstances);

	TexturedQuadPL::ssboObjectInstanceData& item = texturedQuadInstanceData[texturedQuadInstanceIndex % allocationSettings.TexturedQuad_MaxInstances];
	item.uvMin = glm::vec2(0.0f);
	item.uvMax = glm::vec2(1.0f);
	item.translation = pos;
	item.scale = glm::vec2(sprite->resolution.x / sprite->resolution.y * height, height);
	item.rotation = rotation;
	item.tex = sprite->textureID;

	if (sprite->atlas.size() > 0) {
		auto atEntry = sprite->atlas[atlasIndex];
		item.uvMin = atEntry.uv_min;
		item.uvMax = atEntry.uv_max;
	}

	texturedQuadInstanceIndex++;
}
inline void Drawlist::AddCenteredSprite(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprID);
	AddCenteredSprite(s, atlasIndex, pos, height, rotation);
}
inline void Drawlist::AddCenteredSprite(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprite);
	AddCenteredSprite(s, atlasIndex, pos, height, rotation);
}
inline void Drawlist::AddSprite(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprID);
	AddCenteredSprite(s, atlasIndex, pos + (s->resolution / 2.0f) * (height / s->resolution.y), height, rotation);
}
inline void Drawlist::AddSprite(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprite);
	AddCenteredSprite(s, atlasIndex, pos + (s->resolution / 2.0f) * (height / s->resolution.y), height, rotation);
}

// idk what to do about this
inline void Drawlist::AddCenteredFramebufferTexture(framebufferID fbID, glm::vec2 pos, float height, float rotation = 0.0f) {

	FramebufferDrawItem item;

	item.fb = fbID;
	item.pos = pos;
	item.height = height;
	item.rotation = rotation;

	framebufferDrawData.push_back(item);
}


inline void Drawlist::AddScreenSpaceText(fontID font, glm::vec2 position, glm::vec4 color, std::string text) {

	assert(textInstanceIndex < allocationSettings.Text_MaxStrings);

	screenSpaceTextDrawItem& item = textInstanceData[textInstanceIndex % allocationSettings.Text_MaxStrings];
	item.font = font;
	item.text = text;
	item.header.color = color;
	item.header.position = position;
	item.header.rotation = 0.0f;
	item.header.textLength = text.length();

	textInstanceIndex++;
};

inline void Drawlist::AddScreenSpaceText(fontID font, glm::vec2 position, glm::vec4 color, const char* fmt, ...) {

	char buffer[TEXTPL_maxTextLength];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	AddScreenSpaceText(font, position, color, std::string(buffer));
};