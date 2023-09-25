#pragma once

#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <set>
#include <memory>
#include <string>
#include <imgui.h>
#include <optional>

#include <box2d/box2d.h>

#include "texture.h"
#include "typedefs.h"
#include "VKEngine.h"
#include "IDGenerator.h"
#include "Prefab.h"
#include "Sprite.h"
#include "Scene.h"
#include "ResourceManager.h"

#include <assetPack/Package_generated.h>

template<typename I, typename T>
struct MapProxy {
	using Iterator = std::unordered_map<I, T>::iterator;

	Iterator b;
	Iterator e;

	MapProxy(Iterator begin, Iterator end) : b(begin), e(end) {}

	Iterator begin() const { return b; }
	Iterator end() const { return e; }
};

class AssetManager {

public:

	class ChangeFlags {
	public:

		bool SpritesAdded() { return _spritesAdded; };
		bool FontsAdded() { return _fontsAdded; };
		bool TextureFiltersChanged() { return _textureFiltersChanged; };

		void ClearSpritesAdded() { _spritesAdded = false; };
		void ClearFontsAdded() { _fontsAdded = false; };
		void ClearTextureFilterschanged() { _textureFiltersChanged = false; };

		bool _spritesAdded = false;
		bool _fontsAdded = false;
		bool _textureFiltersChanged = false;
	};

	struct AssetPaths {
		std::string prefabDir;
		std::string assetDir;
		std::string textureSrcDir;
		std::string fontsDir;
	};
	AssetPaths directories;

	const AssetPack::Package * package;

	AssetManager(
		std::shared_ptr<VKEngine> engine,
		AssetPaths directories,
		std::shared_ptr<ResourceManager> resourceManager,
		std::shared_ptr<ChangeFlags> changeFlags)
		:
		rengine(engine),
		resourceManager(resourceManager),
		directories(directories),
		changeFlags(changeFlags)
	{
		createAssetLookups();
	}

	const static spriteID defaultSpriteID = 0;

	/// <summary>
	/// Load sprite asset files
	/// </summary>
	/// <param name="loadResources">load underlying textures into video memory</param>
	void LoadAllSprites(bool loadResources = true);
	void LoadSprite(spriteID spriteID, bool loadResources = true);
	void LoadSprite(std::string name, bool loadResources = true);
	void UnloadSprite(spriteID spriteID, bool freeResources = true);
	std::shared_ptr<Sprite> GetSprite(spriteID id) { return spriteAssets[id]; };
	std::shared_ptr<Sprite> GetSprite(std::string name) { return spriteAssets[loadedSpritesByName[name]]; };
	std::optional<ImTextureID> getSpriteImTextureID(spriteID id) {
		return resourceManager->GetTexture(spriteAssets[id]->textureID)->imTexture;
	};

	/// <summary>
	/// Load all font asset files
	/// </summary>
	/// <param name="loadResources">Load underlying font atlas sprite and textures</param>
	void LoadAllFonts(bool loadResources = true);
	void LoadFont(fontID fontID, bool loadResources = true);
	void LoadFont(std::string name, bool loadResources = true);
	void UnloadFont(fontID fontID, bool freeResources = true);
	std::shared_ptr<Font> GetFont(fontID id) { return fontAssets[id]; };
	fontID GetFontID(std::string name) { 
		assert(loadedFontsByName.contains(name));
		return loadedFontsByName[name]; 
	};

	// load and assemble all prefabs
	void LoadAllPrefabs(std::shared_ptr<b2World> world, bool loadResources);
	void LoadPrefab(std::string name, std::shared_ptr<b2World> world, bool loadResources);
	// void UnloadPrefab(std::string name, bool unloadResources); TODO: should impliment but probably won't ever need
	Prefab GetPrefab(std::string name) { return prefabAssets[name]; };

	void LoadScene(std::string sceneName, std::shared_ptr<b2World> world, bool loadResources = true);
	void UnloadScene(std::string sceneName, bool unloadResources);
	std::shared_ptr<Scene> GetScene(std::string name) { return sceneAssets.find(name)->second; };

	void CreateDefaultSprite(int w, int h, std::vector<uint8_t>& data);
	void UpdateSpritefilter(spriteID id) {

#ifndef  USE_PACKED_ASSETS
		const auto& sprite = spriteAssets[id];
		resourceManager->UpdateTexture(sprite->textureID, sprite->filterMode);
		changeFlags->_textureFiltersChanged = true;

		// update file on disk (used by editor only)
		sprite->serializeJson(spritePathsByID[id]);
#endif
	};

#ifndef PUBLISH
	// Generated assets can be exported with these functions which assign an ID and save the asset to disk
	spriteID ExportSprite(std::string spriteAssetExportPath, std::string imageSourcePath, Sprite unidentified_sprite);
	fontID ExportFont(std::string fontAssetExportPath, std::string spriteAssetExportPath, std::string atlasImageSourcePath, Font unidentified_font, Sprite unidentified_sprite);
	void ExportPrefab(Prefab& prefab, std::string prefabAssetExportPath);
#endif

	auto _getSpriteIterator() { return MapProxy<spriteID, std::shared_ptr<Sprite>>(spriteAssets.begin(), spriteAssets.end()); };
	auto _getFontIterator() { return MapProxy<fontID, std::shared_ptr<Font>>(fontAssets.begin(), fontAssets.end()); };
	auto _getPrefabIterator() { return MapProxy<std::string, Prefab>(prefabAssets.begin(), prefabAssets.end()); };
	size_t _spriteAssetCount() { return spriteAssets.size(); }
	size_t _fontAssetCount() { return fontAssets.size(); }

private:

	std::unordered_map<spriteID, std::shared_ptr<Sprite>>  spriteAssets;
	std::unordered_map<fontID, std::shared_ptr<Font>> fontAssets;
	std::unordered_map<std::string, Prefab> prefabAssets;
	std::unordered_map<std::string, std::shared_ptr<Scene>> sceneAssets;

	std::unordered_map<std::string, spriteID> loadedSpritesByName;
	std::unordered_map<std::string, fontID> loadedFontsByName;

	std::shared_ptr<VKEngine> rengine = nullptr;
	std::shared_ptr<ChangeFlags> changeFlags = nullptr;
	std::shared_ptr<ResourceManager> resourceManager;

	IDGenerator<spriteID> SpriteIDGenerator;
	IDGenerator<fontID> fontIDGenerator;

	// all loaded ran
	bool allLoadedSprites = false;
	bool allLoadedFonts = false;
	bool allLoadedPrefabs = false;

	bool defaultSpriteCreated = false;

#ifdef USE_PACKED_ASSETS
	std::unordered_map<std::string, uint32_t> spriteIndexesByName;
	std::unordered_map<spriteID, uint32_t> spriteIndexesByID;

	std::unordered_map<std::string, uint32_t> fontIndexesByName;
	std::unordered_map<fontID, uint32_t> fontIndexesByID;

	std::unordered_map<std::string, uint32_t> prefabIndexesByName;

	std::unordered_map<std::string, uint32_t> sceneIndexesByName;

	void createAssetLookups() {

		for (size_t i = 0; i < package->sprites()->size(); i++) {
			spriteIndexesByID[package->spriteIDs()->Get(i)] = i;
			spriteIndexesByName[package->spriteNames()->Get(i)->str()] = i;
		}
		for (size_t i = 0; i < package->fonts()->size(); i++){
			fontIndexesByID[package->fontIDs()->Get(i)] = i;
			fontIndexesByName[package->fontNames()->Get(i)->str()] = i;
		}
		for (size_t i = 0; i < package->prefabs()->size(); i++) {
			prefabIndexesByName[package->fontNames()->Get(i)->str()] = i;
		}
		for (size_t i = 0; i < package->scenes()->size(); i++) {
			sceneIndexesByName[package->sceneNames()->Get(i)->str()] = i;
		}
	}
#else
	std::unordered_map<std::string, std::string> spritePathsByName;
	std::unordered_map<spriteID, std::string> spritePathsByID;

	std::unordered_map<std::string, std::string> fontPathsByName;
	std::unordered_map<fontID, std::string> fontPathsByID;

	std::unordered_map<std::string, std::string> prefabPathsByName;

	std::unordered_map<std::string, std::string> scenePathsByName;

	//std::unordered_map<std::string, std::string> ImagePathsByFileName;

	void createAssetLookups() {

		spritePathsByName.clear();
		spritePathsByID.clear();
		fontPathsByName.clear();
		fontPathsByID.clear();
		prefabPathsByName.clear();
		scenePathsByName.clear();
		//ImagePathsByFileName.clear();

		std::vector<std::string> assetFiles = getAllFilesInDirectory(directories.assetDir);
		std::vector<std::string> prefabFiles = getAllFilesInDirectory(directories.prefabDir);
		std::vector<std::string> imageFiles = getAllFilesInDirectory(directories.textureSrcDir);

		// combine all three vectors into one
		assetFiles.insert(assetFiles.end(), prefabFiles.begin(), prefabFiles.end());
		assetFiles.insert(assetFiles.end(), imageFiles.begin(), imageFiles.end());

		for (auto& f : assetFiles) {

			size_t lastindex = f.find_last_of(".");
			std::string extension = f.substr(lastindex, f.length() - 1);


			if (extension == Sprite_extension) {

				auto sprite = Sprite::deserializeJson(f);

				// responsibility of engine to create the default sprite
				if (sprite->ID == defaultSpriteID)
					continue;

				spritePathsByID[sprite->ID] = f;
				spritePathsByName[sprite->name] = f;
			}
			else if (extension == Font_extension) {
				auto font = Font::deserializeBinary(f);
				fontPathsByID[font->ID] = f;
				fontPathsByName[font->name] = f;
			}
			else if (extension == Prefab_extension) {
				auto name = Prefab::peakJsonName(f);
				prefabPathsByName[name] = f;
			}
			else if (extension == Scene_extension) {
				auto name = Scene::peakJsonName(f);
				scenePathsByName[name] = f;
			}
			//else {
			//	for (auto& s : ResourceManager_supportedExtensions) {
			//		if (extension == s) {
			//			std::string name = getFileName(f);
			//			ImagePathsByFileName[name] = f;
			//		}
			//	}
			//}
		}
	};
#endif

	void loadPrefabResources(Prefab& prefab);
	void loadSceneResources(SceneData& sceneData);

};


