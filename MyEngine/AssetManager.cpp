#include "stdafx.h"

#include "AssetManager.h"
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <filesystem>
#include <vector>
#include "ECS.h";

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

#ifdef USE_PACKED_ASSETS
	if (packageAssets->sprites() == nullptr)
		return;

	for (int i = 0; i < packageAssets->sprites()->size(); i++) {
		// IDEA:  
		// auto pack = packageAssets->sprites()->Get(i);
		// auto [iter, inserted] spriteAssets.emplace(pack>ID());
		/// Also, why not make the deserialize functions constructers to simplify emplace usage?
		auto sprite = Sprite::deserializeFlatbuffer(packageAssets->sprites()->Get(i));
#else
	for (auto& [ID, path] : spritePathsByID) {
		auto [iter, inserted] = spriteAssets.emplace(std::piecewise_construct, std::forward_as_tuple(ID), std::tuple<>());
		Sprite* sprite = &iter->second;
		Sprite::deserializeJson(path, sprite);
#endif

		loadedSpritesByName[sprite->name] = sprite->ID;
		SpriteIDGenerator.Input(sprite->ID);

		if (loadResources && resourceManager->HasTexture(sprite->textureID) == false) {
#ifdef USE_PACKED_ASSETS
			uint32_t fileSize = resourceFileSizes[sprite->imageFileName];
			vector<uint8_t> fileData;
			fileData.resize(fileSize);
			GetPackedResourceData(fileData.data(), resourceFileOffsets[sprite->imageFileName], fileSize);
			resourceManager->LoadTexture(fileData.data(), fileSize, sprite->imageFileName, sprite->filterMode, usingEditor);
#else
			resourceManager->LoadTexture(directories.assetDir + sprite->imageFileName, sprite->filterMode, usingEditor);
#endif
		}
	}
}


void AssetManager::LoadSprite(spriteID spriteID, bool loadResources) {

	auto [iter, inserted] = spriteAssets.emplace(std::piecewise_construct, std::forward_as_tuple(spriteID), std::tuple<>());
	Sprite* sprite = &spriteAssets.at(spriteID);

#ifdef USE_PACKED_ASSETS
	Sprite::deserializeFlatbuffer(packageAssets->sprites()->Get(spriteIndexesByID[spriteID]), sprite);
#else
	assert(spriteAssets.contains(spriteID) == false);
	Sprite::deserializeJson(spritePathsByID.at(spriteID), sprite);
#endif

	loadedSpritesByName[sprite->name] = sprite->ID;

	if (loadResources && resourceManager->HasTexture(sprite->textureID) == false) {
#ifdef USE_PACKED_ASSETS
		uint32_t fileSize = resourceFileSizes[sprite->imageFileName];
		vector<uint8_t> fileData;
		fileData.resize(fileSize);
		GetPackedResourceData(fileData.data(), resourceFileOffsets[sprite->imageFileName], fileSize);
		resourceManager->LoadTexture(fileData.data(), fileSize, sprite->imageFileName, sprite->filterMode, usingEditor);
#else
		resourceManager->LoadTexture(directories.assetDir + sprite->imageFileName, sprite->filterMode, usingEditor);
#endif
	}
}

// slowest method since sprite must be coppied to read ID before inserting into asset map (at least for json?)
void AssetManager::LoadSprite(std::string name, bool loadResources) {

#ifdef USE_PACKED_ASSETS
	auto sprite = Sprite::deserializeFlatbuffer(packageAssets->sprites()->Get(spriteIndexesByName[name]));
#else
	assert(spritePathsByName.contains(name) == true);
	Sprite tmpSprite;
	Sprite::deserializeJson(spritePathsByName[name], &tmpSprite);
	auto [iter, inserted] = spriteAssets.emplace(tmpSprite.ID, tmpSprite);
	Sprite* sprite = &iter->second;
#endif

	loadedSpritesByName[sprite->name] = sprite->ID;

	assert(spriteAssets.contains(sprite->ID) == false);

	if (loadResources && resourceManager->HasTexture(sprite->textureID) == false) {

#ifdef USE_PACKED_ASSETS
		uint32_t fileSize = resourceFileSizes[sprite->imageFileName];
		vector<uint8_t> fileData;
		fileData.resize(fileSize);
		GetPackedResourceData(fileData.data(), resourceFileOffsets[sprite->imageFileName], fileSize);
		resourceManager->LoadTexture(fileData.data(), fileSize, sprite->imageFileName, sprite->filterMode, usingEditor);
#else
		resourceManager->LoadTexture(directories.assetDir + sprite->imageFileName, sprite->filterMode, usingEditor);
#endif
	}
}

void AssetManager::UnloadSprite(spriteID spriteID, bool freeResources) {
	const auto& sprite = spriteAssets[spriteID];
	if (freeResources)
		resourceManager->SetTextureFreeable(sprite.textureID);
	//resourceManager->FreeTexture(sprite->textureID);

	loadedSpritesByName.erase(sprite.name);
	spriteAssets.erase(spriteID);
}

/// <summary>
/// Load all font asset files
/// </summary>
/// <param name="loadResources">Load underlying font atlas sprite and textures</param>
void AssetManager::LoadAllFonts(bool loadResources) {

	assert(allLoadedFonts == false);
	allLoadedFonts = true;

#ifdef USE_PACKED_ASSETS
	if (packageAssets->fonts() == nullptr)
		return;

	for (int i = 0; i < packageAssets->fonts()->size(); i++) {
		auto font = Font::deserializeFlatbuffer(packageAssets->fonts()->Get(i));
#else
	for (auto& [fontID, path] : fontPathsByID) {
		auto [iter, inserted] = fontAssets.emplace(std::piecewise_construct, std::forward_as_tuple(fontID), std::tuple<>());
		Font* font = &iter->second;
		Font::deserializeBinary(path, font);
#endif

		loadedFontsByName[font->name] = font->ID;
		fontIDGenerator.Input(font->ID);

		if (loadResources && spriteAssets.contains(font->atlas) == false)
			LoadSprite(font->atlas, true);
	}
}

void AssetManager::LoadFont(fontID fontID, bool loadResources) {
	assert(fontAssets.contains(fontID) == false);

	auto [iter, inserted] = fontAssets.emplace(std::piecewise_construct, std::forward_as_tuple(fontID), std::tuple<>());
	Font* font = &iter->second;

#ifdef USE_PACKED_ASSETS
	auto font = Font::deserializeFlatbuffer(packageAssets->fonts()->Get(fontIndexesByID[fontID]));
#else
	Font::deserializeBinary(fontPathsByID[fontID], font);
#endif

	loadedFontsByName[font->name] = font->ID;

	if (loadResources && spriteAssets.contains(font->atlas) == false)
		LoadSprite(font->atlas, true);
}

void AssetManager::LoadFont(std::string name, bool loadResources) {

#ifdef USE_PACKED_ASSETS
	auto font = Font::deserializeFlatbuffer(packageAssets->fonts()->Get(fontIndexesByName[name]));
#else
	assert(fontPathsByName.contains(name) == true);
	Font tmpFont;
	Font::deserializeBinary(fontPathsByName[name], &tmpFont);
	auto [iter, inserted] = fontAssets.emplace(tmpFont.ID, tmpFont);
	Font* font = &iter->second;
#endif

	loadedFontsByName[font->name] = font->ID;

	assert(fontAssets.contains(font->ID) == false);

	if (loadResources && spriteAssets.contains(font->atlas) == false)
		LoadSprite(font->atlas, true);
}
void AssetManager::UnloadFont(fontID fontID, bool freeResources) {
	const auto& font = fontAssets[fontID];
	if (freeResources) {
		UnloadSprite(font.atlas, true);
	};
	fontAssets.erase(fontID);
	loadedFontsByName.erase(font.name);
}

// load and assemble all prefabs
void AssetManager::LoadAllPrefabs(bool loadResources) {

	assert(allLoadedPrefabs == false);
	allLoadedPrefabs = true;

#ifdef USE_PACKED_ASSETS
	if (packageAssets->prefabs() == nullptr)
		return;

	for (int i = 0; i < packageAssets->prefabs()->size(); i++) {
		auto prefab = Prefab::deserializeFlatbuffer(packageAssets->prefabs()->Get(i));
		prefabAssets[prefab.name] = prefab;
#else
	for (auto& [name, path] : prefabPathsByName) {

		auto [iter, inserted] = prefabAssets.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::tuple<>());
		Prefab::deserializeJson(path, &iter->second);
#endif
	}
}


void AssetManager::LoadPrefab(std::string name, bool loadResources) {
#ifdef USE_PACKED_ASSETS
	prefabAssets[name] = Prefab::deserializeFlatbuffer(packageAssets->prefabs()->Get(prefabIndexesByName[name]));
#else
	assert(prefabPathsByName.contains(name));

	if (prefabAssets.contains(name))
		// overwrite loaded asset
		prefabAssets.erase(name);

	auto [iter, inserted] = prefabAssets.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::tuple<>());
	Prefab::deserializeJson(prefabPathsByName.at(name), &iter->second);

#endif
	if (loadResources)
		loadPrefabResources(prefabAssets[name]);
}


void AssetManager::LoadScene(std::string sceneName, bool loadResources) {

#ifdef USE_PACKED_ASSETS
	auto scene = Scene::deserializeFlatbuffers(packageAssets->scenes()->Get(sceneIndexesByName[sceneName]));
#else
	auto scene = Scene::deserializeJson(scenePathsByName[sceneName]);
	assert(scenePathsByName.contains(sceneName));
#endif
	sceneAssets.insert_or_assign(sceneName, scene);
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



void AssetManager::loadPrefabResources(Prefab & prefab) {

	// assuming already loaded resources containing assets have already loaded in their respective resources
	loadSceneResources(prefab.sceneData);
}

void AssetManager::loadSceneResources(SceneData & sceneData) {

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

	resourceManager->SetTextureFreeable(id);
	//resourceManager->FreeTexture(id);

	createAssetLookups();

	return unidentified_sprite.ID;
}
fontID AssetManager::ExportFont(std::string fontAssetExportPath, std::string spriteAssetExportPath, std::string atlasImageSourcePath, Font unidentified_font, Sprite unidentified_sprite) {

	checkAppend(fontAssetExportPath, Font_extension);

	spriteID sprID = ExportSprite(spriteAssetExportPath, atlasImageSourcePath, unidentified_sprite);
	unidentified_font.ID = fontIDGenerator.GenerateID(unidentified_font.name);
	unidentified_font.atlas = sprID;
	unidentified_font.serializeBinary(fontAssetExportPath);
	unidentified_font.serializeJson(fontAssetExportPath + ".fjson"); // for automatic packer only

	createAssetLookups();

	return unidentified_font.ID;
}
void AssetManager::ExportPrefab(Prefab & prefab, std::string prefabAssetExportPath) {

	prefab.serializeJson(prefabAssetExportPath);

	createAssetLookups();
}

#endif


void AssetManager::CreateDefaultSprite(int w, int h, std::vector<uint8_t>&data) {

	assert(defaultSpriteCreated == false);
	defaultSpriteCreated = true;

	texID texid = resourceManager->GenerateTexture(w, h, data, FilterMode::Nearest, true);
	const auto& tex = resourceManager->GetTexture(texid);

	auto [iter, inserted] = spriteAssets.emplace(std::piecewise_construct, std::forward_as_tuple(defaultSpriteID), std::tuple<>());
	Sprite* sprite = &iter->second;

	sprite->name = "default";
	sprite->imageFileName = "N/A";
	sprite->textureID = texid;
	sprite->ID = defaultSpriteID;
	sprite->resolution = glm::vec2(tex->resolutionX, tex->resolutionY);

	SpriteIDGenerator.Input(defaultSpriteID);
}

