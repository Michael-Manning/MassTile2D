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

inline void Drawlist::AddScreenCenteredSpaceTexture(Sprite* sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {

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
inline void Drawlist::AddScreenCenteredSpaceTexture(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprID);
	AddScreenCenteredSpaceTexture(s, atlasIndex, pos, height, rotation);
}
inline void Drawlist::AddScreenCenteredSpaceTexture(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprite);
	AddScreenCenteredSpaceTexture(s, atlasIndex, pos, height, rotation);
}
inline void Drawlist::AddScreenSpaceTexture(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprID);
	AddScreenCenteredSpaceTexture(s, atlasIndex, pos + (s->resolution / 2.0f) * (height / s->resolution.y), height, rotation);
}
inline void Drawlist::AddScreenSpaceTexture(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprite);
	AddScreenCenteredSpaceTexture(s, atlasIndex, pos + (s->resolution / 2.0f) * (height / s->resolution.y), height, rotation);
}

// idk what to do about this
inline void AddScreenCenteredSpaceFramebufferTexture(framebufferID fbID, glm::vec2 pos, float height, float rotation = 0.0f) {

	auto fb = resourceManager->GetFramebuffer(fbID);

	float w = fb->extents[rengine->currentFrame].width;
	float h = fb->extents[rengine->currentFrame].height;

	assert(screenSpaceTextureGPUIndex < TexturedQuadPL_MAX_OBJECTS);
	TexturedQuadPL::ssboObjectInstanceData* item = screenSpaceTextureGPUBuffer + screenSpaceTextureGPUIndex++;

	item->uvMin = glm::vec2(0.0f);
	item->uvMax = glm::vec2(1.0f);
	item->translation = pos;
	item->scale = glm::vec2((w / h) * height, height);
	item->rotation = rotation;
	item->tex = fb->textureIDs[rengine->currentFrame];
}


inline void addScreenSpaceText(fontID font, glm::vec2 position, glm::vec4 color, std::string text) {
	screenSpaceTextDrawItem item;
	item.font = font;
	item.text = text;
	item.header.color = color;
	item.header.position = position;
	item.header.rotation = 0.0f;
	item.header.textLength = text.length();
	screenSpaceTextDrawlist.push_back(item);
};

void addScreenSpaceText(fontID font, glm::vec2 position, glm::vec4 color, const char* fmt, ...) {

	char buffer[TEXTPL_maxTextLength];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	std::string result = buffer;

	screenSpaceTextDrawItem item;
	item.font = font;
	item.text = result;
	item.header.color = color;
	item.header.position = position;
	item.header.rotation = 0.0f;
	item.header.textLength = result.length();
	screenSpaceTextDrawlist.push_back(item);
};