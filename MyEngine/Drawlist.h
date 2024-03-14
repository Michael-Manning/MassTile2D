#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>

#include "pipelines.h"
#include "Settings.h"
#include "AssetManager.h"

class Engine;

class Drawlist {

public:

	Drawlist(DrawlistAllocationConfiguration allocationSettings, AssetManager* assetManager)
		: allocationSettings(allocationSettings), assetManager(assetManager)
	{

		coloredQuadInstanceData.resize(allocationSettings.ColoredQuad_MaxInstances);
		texturedQuadInstanceData.resize(allocationSettings.TexturedQuad_MaxInstances);
		textInstanceData.resize(allocationSettings.Text_MaxStrings);
	}

	 void AddCenteredQuad(glm::vec4 color, glm::vec2 pos, glm::vec2 scale, float rotation = 0.0f);

	 void AddCenteredSprite(Sprite* sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);
	 void AddCenteredSprite(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);
	 void AddCenteredSprite(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);

	 void AddSprite(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);
	 void AddSprite(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);

	 void AddText(fontID font, glm::vec2 position, glm::vec4 color, std::string text);
	 void AddText(fontID font, glm::vec2 position, glm::vec4 color, const char* fmt, ...);

	 void AddCenteredFramebufferTexture(framebufferID fbID, glm::vec2 pos, float height, float rotation = 0.0f);

private:
	friend Engine;

	void ResetIndexes() {
		coloredQuadInstanceIndex = 0;
		texturedQuadInstanceIndex = 0;
		textInstanceIndex = 0;

		framebufferDrawData.clear();
	}

	const DrawlistAllocationConfiguration allocationSettings;
	AssetManager* assetManager;

	struct FramebufferDrawItem {
		framebufferID fb;
		glm::vec2 pos;
		float height;
		float rotation;
	};

	struct screenSpaceTextDrawItem {
		TextPL::textHeader header;
		std::string text;
		fontID font;
		float scaleFactor = 1.0f;
	};

	// drawlist data
	std::vector<ColoredQuadPL::InstanceBufferData> coloredQuadInstanceData;
	int coloredQuadInstanceIndex = 0;

	std::vector<TexturedQuadPL::ssboObjectInstanceData> texturedQuadInstanceData;
	int texturedQuadInstanceIndex = 0;

	std::vector<screenSpaceTextDrawItem> textInstanceData;
	int textInstanceIndex = 0;


	// doesn't contribute to texture draw limit because I'm assuming I'll create a dedicated pipeline for this which includes effects
	std::vector<FramebufferDrawItem> framebufferDrawData;

};
