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
	};

	AssetManager(std::shared_ptr<VKEngine> engine, AssetPaths directories) : rengine(engine) {
		this->directories = directories;
	}

	// currently only used for generating default texture. Don't supply inputID for automatic ID
	texID loadTexture(int w, int h, std::vector<uint8_t>& data, FilterMode filterMode, bool imGuiTexure = true, texID inputID = -1);

	texID loadTexture(std::string imagePath, FilterMode filterMode, bool imGuiTexure = true);

	void updateTexture(texID id, FilterMode filterMode);

	std::unordered_map<texID, Texture> textureAssets;
	std::unordered_map<texID, std::string> imageSources;

	std::unordered_map<std::string, Prefab> prefabs;
	void loadPrefabs(std::shared_ptr<b2World> world);



	std::unordered_map<spriteID, std::shared_ptr<Sprite>>  spriteAssets;

	void loadSpriteAssets(std::set<spriteID> ids);
	void loadAllSprites();

	std::shared_ptr<Sprite> GenerateSprite(std::string imagePath, FilterMode filterMode = FilterMode::Linear, bool genImgui = true);

	bool spritesAdded = false; // dirty flag used by engine to bind new textures to 
	bool filterModesChanged = false;

	void CreateDefaultSprite(int w, int h, std::vector<uint8_t>& data);

	spriteID defaultSprite = 0;

	AssetPaths directories;

private:

	IDGenerator<texID> TextureIDGenerator;
	IDGenerator<spriteID> SpriteGenerator;

	std::shared_ptr<VKEngine> rengine;

	// all loaded ran
	bool allLoaded = false;
};


