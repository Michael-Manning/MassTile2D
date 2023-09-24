#include "AssetManager.h"
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <filesystem>
#include <vector>

#include "Utils.h"
#include "Prefab.h"
#include "sprite.h"
#include "Font.h"

using namespace std;

#ifdef USING_EDITOR
constexpr bool usingEditor = true;
#else
constexpr bool usingEditor = false;
#endif

void AssetManager::LoadAllSprites(bool loadResources) {

	assert(allLoadedSprites == false);
	allLoadedSprites = true;

	for (auto& [ID, path] : spritePathsByID) {

		auto sprite = Sprite::deserializeJson(path);

		spriteAssets[sprite->ID] = sprite;
		loadedSpritesByName[sprite->name] = sprite->ID;
		SpriteIDGenerator.Input(sprite->ID);

		if (loadResources && resourceManager->HasTexture(sprite->textureID) == false)
			resourceManager->LoadTexture(directories.textureSrcDir + sprite->imageFileName, sprite->filterMode, usingEditor);
	}

	changeFlags->_spritesAdded = true;
}


void AssetManager::LoadSprite(spriteID spriteID, bool loadResources) {

	assert(spriteAssets.contains(spriteID) == false);

	auto sprite = Sprite::deserializeJson(spritePathsByID[spriteID]);
	spriteAssets[sprite->ID] = sprite;
	loadedSpritesByName[sprite->name] = sprite->ID;

	if (loadResources && resourceManager->HasTexture(sprite->textureID) == false)
		resourceManager->LoadTexture(directories.textureSrcDir + sprite->imageFileName, sprite->filterMode, usingEditor);

	changeFlags->_spritesAdded = true;
}

void AssetManager::LoadSprite(std::string name, bool loadResources) {

	assert(spritePathsByName.contains(name) == true);
	auto sprite = Sprite::deserializeJson(spritePathsByName[name]);
	spriteAssets[sprite->ID] = sprite;
	loadedSpritesByName[sprite->name] = sprite->ID;

	assert(spriteAssets.contains(sprite->ID) == false);

	if (loadResources && resourceManager->HasTexture(sprite->textureID) == false)
		resourceManager->LoadTexture(directories.textureSrcDir + sprite->imageFileName, sprite->filterMode, usingEditor);

	changeFlags->_spritesAdded = true;
}

void AssetManager::UnloadSprite(spriteID spriteID, bool freeResources) {
	auto& sprite = spriteAssets[spriteID];
	if (freeResources) 
		resourceManager->FreeTexture(sprite->textureID);
	
	loadedSpritesByName.erase(sprite->name);
	spriteAssets.erase(spriteID);
}

/// <summary>
/// Load all font asset files
/// </summary>
/// <param name="loadResources">Load underlying font atlas sprite and textures</param>
void AssetManager::LoadAllFonts(bool loadResources) {

	assert(allLoadedFonts == false);
	allLoadedFonts = true;

	auto fontFiles = getAllFilesInDirectory(std::filesystem::path(directories.assetDir));

	for (auto& [fontID, path] : fontPathsByID) {
		auto font = Font::deserializeBinary(path);

		fontAssets[font->ID] = font;
		loadedFontsByName[font->name] = font->ID;
		fontIDGenerator.Input(font->ID);

		if (loadResources && spriteAssets.contains(font->atlas) == false)
			LoadSprite(font->atlas, true);
	}

	changeFlags->_fontsAdded = true;
}

void AssetManager::LoadFont(fontID fontID, bool loadResources) {
	assert(fontAssets.contains(fontID) == false);

	auto font = Font::deserializeBinary(fontPathsByID[fontID]);
	fontAssets[font->ID] = font;
	loadedFontsByName[font->name] = font->ID;

	if (loadResources && spriteAssets.contains(font->atlas) == false)
		LoadSprite(font->atlas, true);

	changeFlags->_fontsAdded = true;
}

void AssetManager::LoadFont(std::string name, bool loadResources) {

	assert(fontPathsByName.contains(name) == true);

	auto font = Font::deserializeBinary(fontPathsByName[name]);
	fontAssets[font->ID] = font;
	loadedFontsByName[font->name] = font->ID;

	assert(fontAssets.contains(font->ID) == false);

	if (loadResources && spriteAssets.contains(font->atlas) == false)
		LoadSprite(font->atlas, true);

	changeFlags->_fontsAdded = true;
}
void AssetManager::UnloadFont(fontID fontID, bool freeResources) {
	auto& font = fontAssets[fontID];
	if (freeResources) {
		UnloadSprite(font->atlas, true);
	};
	fontAssets.erase(fontID);
	loadedFontsByName.erase(font->name);
}

// load and assemble all prefabs
void AssetManager::LoadAllPrefabs(std::shared_ptr<b2World> world, bool loadResources) {

	assert(allLoadedPrefabs == false);
	allLoadedPrefabs = true;

	for (auto& [name, path] : prefabPathsByName) {
		prefabAssets[name] = Prefab::deserializeJson(path, world);
	}
}


void AssetManager::LoadPrefab(std::string name, std::shared_ptr<b2World> world, bool loadResources) {
	assert(prefabPathsByName.contains(name));
	prefabAssets[name] = Prefab::deserializeJson(prefabPathsByName[name], world);
	if (loadResources)
		loadPrefabResources(prefabAssets[name]);
}


void AssetManager::LoadScene(std::string sceneName, std::shared_ptr<b2World> world, bool loadResources) {
	assert(scenePathsByName.contains(sceneName));
	auto scene = Scene::deserializeJson(scenePathsByName[sceneName], world);
	sceneAssets.emplace(sceneName, scene);
	if (loadResources)
		loadSceneResources(scene->sceneData);
}
void AssetManager::UnloadScene(std::string sceneName, bool unloadResources) {
	assert(sceneAssets.contains(sceneName));
	if (unloadResources) {
		auto& scene = sceneAssets.find(sceneName)->second;
		for (auto& i : scene->sceneData.getUsedSprites())
			UnloadSprite(i, true);
		for (auto& i : scene->sceneData.getUsedFonts())
			UnloadFont(i, true);
	}
	sceneAssets.erase(sceneName);
}



void AssetManager::loadPrefabResources(Prefab& prefab) {

	// assuming already loaded resources containing assets have already loaded in their respective resources

	if (prefab.spriteRenderer.has_value() && spriteAssets.contains(prefab.spriteRenderer.value().sprite == false))
		LoadSprite(prefab.spriteRenderer.value().sprite, true);

	if (prefab.textRenderer.has_value() && fontAssets.contains(prefab.textRenderer.value().font) == false)
		LoadFont(prefab.textRenderer.value().font, true);
}

void AssetManager::loadSceneResources(SceneData& sceneData) {

	// assuming already loaded resources containing assets have already loaded in their respective resources

	auto usedspriteIDs = sceneData.getUsedSprites();
	for (auto& id : usedspriteIDs) {
		if (spriteAssets.contains(id) == false)
			LoadSprite(id, true);
	}

	auto usedfontIDs = sceneData.getUsedFonts();
	for (auto& id : usedfontIDs) {
		if (fontAssets.contains(id) == false)
			LoadFont(id, true);
	}
}


#ifndef PUBLISH


spriteID AssetManager::ExportSprite(std::string spriteAssetExportPath, std::string imageSourcePath, Sprite unidentified_sprite) {

	// could replace with a function that only grabs metadata from image but who cares since this is an editor only utility
	texID id = resourceManager->LoadTexture(imageSourcePath, FilterMode::Nearest, 0);
	auto texture = resourceManager->GetTexture(id);

	checkAppend(spriteAssetExportPath, Sprite_extension);

	unidentified_sprite.imageFileName = getFileName(imageSourcePath);
	unidentified_sprite.textureID = id;
	unidentified_sprite.resolution = glm::vec2(texture->resolutionX, texture->resolutionY);
	unidentified_sprite.ID = SpriteIDGenerator.GenerateID(unidentified_sprite.name);
	unidentified_sprite.serializeJson(spriteAssetExportPath);

	resourceManager->FreeTexture(id);

	createAssetLookups();

	return unidentified_sprite.ID;
}
fontID AssetManager::ExportFont(std::string fontAssetExportPath, std::string spriteAssetExportPath, std::string atlasImageSourcePath, Font unidentified_font, Sprite unidentified_sprite) {

	checkAppend(fontAssetExportPath, Font_extension);

	spriteID sprID = ExportSprite(spriteAssetExportPath, atlasImageSourcePath, unidentified_sprite);
	unidentified_font.ID = fontIDGenerator.GenerateID(unidentified_font.name);
	unidentified_font.atlas = sprID;
	unidentified_font.serializeBinary(fontAssetExportPath);

	createAssetLookups();

	return unidentified_font.ID;
}
void AssetManager::ExportPrefab(Prefab& prefab, std::string prefabAssetExportPath) {

	prefab.serializeJson(prefabAssetExportPath);

	createAssetLookups();
}

#endif


void AssetManager::CreateDefaultSprite(int w, int h, std::vector<uint8_t>& data) {

	assert(defaultSpriteCreated == false);
	defaultSpriteCreated = true;

	texID texid = resourceManager->GenerateTexture(w, h, data, FilterMode::Nearest, true);
	const auto& tex = resourceManager->GetTexture(texid);

	auto sprite = make_shared<Sprite>();
	sprite->name = "default";
	sprite->imageFileName = "N/A";
	sprite->textureID = texid;
	sprite->ID = defaultSpriteID;
	sprite->resolution = glm::vec2(tex->resolutionX, tex->resolutionY);

	SpriteIDGenerator.Input(defaultSpriteID);

	changeFlags->_spritesAdded = true;

	spriteAssets[defaultSpriteID] = sprite;
}

