#include "stdafx.h"

#include <vector>
#include <string>

#include <glm/glm.hpp>

#include "pipelines.h"
#include "Settings.h"
#include "AssetManager.h"

#include "Drawlist.h"


void Drawlist::AddCenteredQuad(glm::vec4 color, glm::vec2 pos, glm::vec2 scale, float rotation) {

	assert(coloredQuadInstanceIndex < allocationSettings.ColoredQuad_MaxInstances);

	ColoredQuadPL::InstanceBufferData& item = coloredQuadInstanceData[coloredQuadInstanceIndex % allocationSettings.ColoredQuad_MaxInstances];
	item.color = color;
	item.position = pos;
	item.scale = scale;
	item.rotation = rotation;
	item.circle = 0;

	coloredQuadInstanceIndex++;
}

void Drawlist::AddTriangles(std::vector <glm::vec2>&vertices, std::vector<glm::vec4>&triangleColors) {
	
	assert(vertices.size() % 3 == 0);
	assert(coloredTrianglesInstanceIndex < allocationSettings.ColoredTriangle_MaxInstances);
	assert(triangleColors.size() == coloredTriangleVertexData.size() / ColoredTrianglesPL::verticesPerMesh);

	int triangleIndex = coloredTrianglesInstanceIndex * ColoredTrianglesPL::verticesPerMesh;
	for (auto& c : triangleColors)
	{
		coloredTriangleColorData[triangleIndex].color = c;
		triangleIndex++;
	}

	int vertexIndex = coloredTrianglesInstanceIndex * ColoredTrianglesPL::verticesPerMesh;
	for (auto& v : vertices)
	{
		coloredTriangleVertexData[vertexIndex] = Vertex{ .pos = v, .UVCoord = {0, 0} };
		vertexIndex++;
	}

	coloredTrianglesInstanceIndex = triangleIndex;
}

void Drawlist::AddCenteredSprite(Sprite* sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {

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
void Drawlist::AddCenteredSprite(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprID);
	AddCenteredSprite(s, atlasIndex, pos, height, rotation);
}
void Drawlist::AddCenteredSprite(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprite);
	AddCenteredSprite(s, atlasIndex, pos, height, rotation);
}
void Drawlist::AddSprite(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprID);
	AddCenteredSprite(s, atlasIndex, pos + (s->resolution / 2.0f) * (height / s->resolution.y), height, rotation);
}
void Drawlist::AddSprite(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation) {
	auto s = assetManager->GetSprite(sprite);
	AddCenteredSprite(s, atlasIndex, pos + (s->resolution / 2.0f) * (height / s->resolution.y), height, rotation);
}

void Drawlist::AddCenteredFramebufferTexture(framebufferID fbID, glm::vec2 pos, float height, float rotation) {

	FramebufferDrawItem item;

	item.fb = fbID;
	item.pos = pos;
	item.height = height;
	item.rotation = rotation;

	framebufferDrawData.push_back(item);
}


void Drawlist::AddText(fontID font, glm::vec2 position, glm::vec4 color, std::string text) {

	assert(textInstanceIndex < allocationSettings.Text_MaxStrings);
	assert(text.length() < allocationSettings.Text_MaxStringLength);

	screenSpaceTextDrawItem& item = textInstanceData[textInstanceIndex % allocationSettings.Text_MaxStrings];
	item.font = font;
	item.text = text;
	item.header.color = color;
	item.header.position = position;
	item.header.rotation = 0.0f;
	item.header.textLength = glm::min(allocationSettings.Text_MaxStringLength, (uint32_t)text.length());

	textInstanceIndex++;
};


namespace {
	// reusable buffer for formatted strings. Grows if longer strings are used
	std::vector<char> sprintBuffer;
}

void Drawlist::AddText(fontID font, glm::vec2 position, glm::vec4 color, const char* fmt, ...) {

	//va_list args;
	//va_start(args, fmt);
	//vsnprintf(buffer, sizeof(buffer), fmt, args);
	//va_end(args);


	va_list args;
	va_start(args, fmt);

	// First, we use vsnprintf with a size of 0 to calculate the required length of the formatted string
	int requiredLength = vsnprintf(nullptr, 0, fmt, args) + 1; // +1 for null terminator
	va_end(args);

	// Check if our buffer is large enough; if not, resize it
	if (requiredLength > sprintBuffer.size()) {
		sprintBuffer.resize(requiredLength);
	}

	// We need to start the va_list again since we've used it up above
	va_start(args, fmt);
	// Now we actually format the string into our buffer
	vsnprintf(sprintBuffer.data(), sprintBuffer.size(), fmt, args);
	va_end(args);

	// Finally, call the overloaded AddText method with the formatted string
	AddText(font, position, color, std::string(sprintBuffer.data()));

	AddText(font, position, color, std::string(sprintBuffer.data()));
};