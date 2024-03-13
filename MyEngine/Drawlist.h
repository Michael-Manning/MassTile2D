#pragma once

#include <vector>

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

	}

	inline void AddCenteredQuad(glm::vec4 color, glm::vec2 pos, glm::vec2 scale, float rotation = 0.0f);
	inline void AddScreenCenteredSpaceTexture(Sprite* sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);
	inline void AddScreenCenteredSpaceTexture(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);
	inline void AddScreenCenteredSpaceTexture(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);
	inline void AddScreenSpaceTexture(spriteID sprID, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);
	inline void AddScreenSpaceTexture(std::string sprite, int atlasIndex, glm::vec2 pos, float height, float rotation = 0.0f);

	inline void AddScreenCenteredSpaceFramebufferTexture(framebufferID fbID, glm::vec2 pos, float height, float rotation = 0.0f);

private:
	friend Engine;

	void ResetIndexes() {
		coloredQuadInstanceIndex = 0;
		texturedQuadInstanceIndex = 0;
	}

	const DrawlistAllocationConfiguration allocationSettings;
	AssetManager* assetManager;

	std::vector<ColoredQuadPL::InstanceBufferData> coloredQuadInstanceData;
	int coloredQuadInstanceIndex = 0;

	std::vector<TexturedQuadPL::ssboObjectInstanceData> texturedQuadInstanceData;
	int texturedQuadInstanceIndex = 0;

	std::vector<screenSpaceTextDrawItem> screenSpaceTextDrawlist;

	struct screenSpaceTextDrawItem {
		TextPL::textHeader header;
		std::string text;
		fontID font;
		float scaleFactor = 1.0f;
	};
	std::vector<screenSpaceTextDrawItem> screenSpaceTextDrawlist;
};
