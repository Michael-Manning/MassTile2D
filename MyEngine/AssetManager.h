#pragma once

#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <memory>
#include <string>

#include <box2d/box2d.h>

#include "texture.h"
#include "typedefs.h"
#include "VKEngine.h"
#include "IDGenerator.h"
#include "Prefab.h"
#include "Sprite.h"


class AssetManager {

public:

	struct AssetPaths {
		std::string prefabDir;
		std::string assetDir;
		std::string textureSrcDir;
		std::string fontsDir;
		std::string imagesDir;
	};
	AssetPaths directories;

	AssetManager(std::shared_ptr<VKEngine> engine, AssetPaths directories) : rengine(engine) {
		this->directories = directories;
	}

	// currently only used for generating default texture. Don't supply inputID for automatic ID
	texID loadTexture(int w, int h, std::vector<uint8_t>& data, FilterMode filterMode, bool imGuiTexure = true, texID inputID = -1);

	texID loadTexture(std::string imagePath, FilterMode filterMode, bool imGuiTexure = true);

	void updateTexture(texID id, FilterMode filterMode);

	fontID addFont(Font font);
	void addFont(Font font, fontID inputID);

	std::unordered_map<texID, Texture> textureAssets;
	std::unordered_map<texID, std::string> imageSources;
	std::unordered_map<spriteID, std::shared_ptr<Sprite>>  spriteAssets;
	std::unordered_map<std::string, Prefab> prefabs;
	std::unordered_map<fontID, std::shared_ptr<Font>> fontAssets;

	void loadPrefabs(std::shared_ptr<b2World> world);
	void loadSpriteAssets(std::set<spriteID> ids);
	void loadAllSprites();
	void loadFontAssets(std::set<fontID> ids);
	void loadAllFonts();

	std::shared_ptr<Sprite> GenerateSprite(std::string imagePath, FilterMode filterMode = FilterMode::Linear, bool genImgui = true);
	void CreateDefaultSprite(int w, int h, std::vector<uint8_t>& data);
	spriteID defaultSprite = 0;


	bool spritesAdded = false; // dirty flag used by engine to bind new textures to 
	bool filterModesChanged = false;
private:

	IDGenerator<texID> TextureIDGenerator;
	IDGenerator<spriteID> SpriteGenerator;
	IDGenerator<fontID> fontGenerator;

	std::shared_ptr<VKEngine> rengine;

	// all loaded ran
	bool allLoaded = false;
};


