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
#include "Scene.h"

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

		auto pack = packageAssets->sprites()->Get(i);
		auto [iter, inserted] = spriteAssets.emplace(pack->ID(), pack);
		Sprite* sprite = &iter->second;
#else
	for (auto& [ID, path] : spritePathsByID) {
		auto [iter, inserted] = spriteAssets.emplace(std::piecewise_construct, std::forward_as_tuple(ID), std::tuple<>());
		Sprite* sprite = &iter->second;
		Sprite::deserializeJson(path, sprite);
#endif

		if (loadResources && resourceManager->HasTexture(sprite->imageFileName) == false) {
#ifdef USE_PACKED_ASSETS
			uint32_t fileSize = resourceFileSizes.at(sprite->imageFileName);
			vector<uint8_t> fileData;
			fileData.resize(fileSize);
			GetPackedResourceData(fileData.data(), resourceFileOffsets.at(sprite->imageFileName), fileSize);
			resourceManager->LoadTexture(fileData.data(), fileSize, sprite->imageFileName, sprite->filterMode, usingEditor);
#else
			resourceManager->LoadTexture(directories.assetDir + sprite->imageFileName, sprite->filterMode, usingEditor);
#endif
		}

		sprite->textureID = resourceManager->GetTextureID(sprite->imageFileName);

		loadedSpritesByName[sprite->name] = sprite->ID;
		SpriteIDGenerator.Input(sprite->ID);
	}
}


void AssetManager::LoadSprite(spriteID spriteID, bool loadResources) {


#ifdef USE_PACKED_ASSETS
	int index = spriteIndexesByID.at(spriteID);
	auto [iter, inserted] = spriteAssets.emplace(spriteID, packageAssets->sprites()->Get(index));
	Sprite* sprite = &iter->second;
#else
	auto [iter, inserted] = spriteAssets.emplace(std::piecewise_construct, std::forward_as_tuple(spriteID), std::tuple<>());
	Sprite* sprite = &iter->second;
	Sprite::deserializeJson(spritePathsByID.at(spriteID), sprite);
#endif

	if (loadResources && resourceManager->HasTexture(sprite->imageFileName) == false) {
#ifdef USE_PACKED_ASSETS
		uint32_t fileSize = resourceFileSizes.at(sprite->imageFileName);
		vector<uint8_t> fileData;
		fileData.resize(fileSize);
		GetPackedResourceData(fileData.data(), resourceFileOffsets.at(sprite->imageFileName), fileSize);
		resourceManager->LoadTexture(fileData.data(), fileSize, sprite->imageFileName, sprite->filterMode, usingEditor);
#else
		resourceManager->LoadTexture(directories.assetDir + sprite->imageFileName, sprite->filterMode, usingEditor);
#endif
	}

	sprite->textureID = resourceManager->GetTextureID(sprite->imageFileName);

	loadedSpritesByName[sprite->name] = sprite->ID;
//	SpriteIDGenerator.Input(sprite->ID);
}

// slowest method since sprite must be coppied to read ID before inserting into asset map (at least for json?)
void AssetManager::LoadSprite(std::string name, bool loadResources) {

#ifdef USE_PACKED_ASSETS
	int index = spriteIndexesByName.at(name);
	auto pack = packageAssets->sprites()->Get(index);
	auto [iter, inserted] = spriteAssets.emplace(pack->ID(), pack);
	Sprite* sprite = &iter->second;
#else
	assert(spritePathsByName.contains(name) == true);
	Sprite tmpSprite;
	Sprite::deserializeJson(spritePathsByName[name], &tmpSprite);
	auto [iter, inserted] = spriteAssets.emplace(tmpSprite.ID, tmpSprite);
	Sprite* sprite = &iter->second;
#endif

	if (loadResources && resourceManager->HasTexture(sprite->imageFileName) == false) {
#ifdef USE_PACKED_ASSETS
		uint32_t fileSize = resourceFileSizes[sprite->imageFileName];
		vector<uint8_t> fileData;
		fileData.resize(fileSize);
		GetPackedResourceData(fileData.data(), resourceFileOffsets.at(sprite->imageFileName), fileSize);
		resourceManager->LoadTexture(fileData.data(), fileSize, sprite->imageFileName, sprite->filterMode, usingEditor);
#else
		resourceManager->LoadTexture(directories.assetDir + sprite->imageFileName, sprite->filterMode, usingEditor);
#endif
	}

	sprite->textureID = resourceManager->GetTextureID(sprite->imageFileName);

	loadedSpritesByName[sprite->name] = sprite->ID;
	SpriteIDGenerator.Input(sprite->ID);
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
		auto pack = packageAssets->fonts()->Get(i);
		auto [iter, inserted] = fontAssets.emplace(pack->ID(), pack);
		Font* font = &iter->second;
#else
	for (auto& [fontID, path] : fontPathsByID) {
		auto [iter, inserted] = fontAssets.emplace(std::piecewise_construct, std::forward_as_tuple(fontID), std::tuple<>());
		Font* font = &iter->second;
		Font::deserializeBinary(path, font);
#endif

		loadedFontsByName.insert({font->name, font->ID });
		fontIDGenerator.Input(font->ID);

		if (loadResources && spriteAssets.contains(font->atlas) == false)
			LoadSprite(font->atlas, true);
	}
}

void AssetManager::LoadFont(fontID fontID, bool loadResources) {
	assert(fontAssets.contains(fontID) == false);


#ifdef USE_PACKED_ASSETS
	int index = fontIndexesByID.at(fontID);
	auto [iter, inserted] = fontAssets.emplace(fontID, packageAssets->fonts()->Get(index));
	Font* font = &iter->second;

#else
	auto [iter, inserted] = fontAssets.emplace(std::piecewise_construct, std::forward_as_tuple(fontID), std::tuple<>());
	Font* font = &iter->second;
	Font::deserializeBinary(fontPathsByID[fontID], font);
#endif

	loadedFontsByName[font->name] = font->ID;

	if (loadResources && spriteAssets.contains(font->atlas) == false)
		LoadSprite(font->atlas, true);
}

void AssetManager::LoadFont(std::string name, bool loadResources) {

#ifdef USE_PACKED_ASSETS
	int index = fontIndexesByName.at(name);
	auto  pack = packageAssets->fonts()->Get(index);
	auto [iter, inserted] = fontAssets.emplace(pack->ID(), pack);
	Font* font = &iter->second;
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

		auto pack = packageAssets->prefabs()->Get(i);
		prefabAssets.emplace(pack->name()->str(), pack);
#else
	for (auto& [name, path] : prefabPathsByName) {

		auto [iter, inserted] = prefabAssets.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::tuple<>());
		Prefab::deserializeJson(path, &iter->second);
#endif
	}
}


void AssetManager::LoadPrefab(std::string name, bool loadResources) {
#ifdef USE_PACKED_ASSETS
	prefabAssets.emplace(name, packageAssets->prefabs()->Get(prefabIndexesByName.at(name)));
#else
	assert(prefabPathsByName.contains(name));

	if (prefabAssets.contains(name))
		// overwrite loaded asset
		prefabAssets.erase(name);

	auto [iter, inserted] = prefabAssets.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::tuple<>());
	Prefab::deserializeJson(prefabPathsByName.at(name), &iter->second);

#endif
	if (loadResources)
		loadPrefabResources(prefabAssets.at(name));
}


void AssetManager::LoadScene(std::string sceneName, bool loadResources) {

#ifdef USE_PACKED_ASSETS
	auto scene = make_shared<Scene>(packageAssets->scenes()->Get(sceneIndexesByName[sceneName]), this);
#else
	assert(scenePathsByName.contains(sceneName));

	// uncomment if switching to emplacement
	//if (sceneAssets.contains(sceneName))
	//	// overwrite loaded asset
	//	sceneAssets.erase(sceneName);

	auto scene = Scene::deserializeJson(scenePathsByName.at(sceneName), this);
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
	//texID id = resourceManager->LoadTexture(imageSourcePath, FilterMode::Nearest, 0);
	//auto texture = resourceManager->GetTexture(id);

	checkAppend(spriteAssetExportPath, Sprite_extension);

	unidentified_sprite.imageFileName = getFileName(imageSourcePath);
	//unidentified_sprite.textureID = id;
	unidentified_sprite.resolution = resourceManager->GetImageFileResolution(imageSourcePath); //glm::vec2(texture->resolutionX, texture->resolutionY);
	unidentified_sprite.ID = SpriteIDGenerator.GenerateID(unidentified_sprite.name);
	unidentified_sprite.serializeJson(spriteAssetExportPath);

	//resourceManager->SetTextureFreeable(id);
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

void AssetManager::ExportScene(std::shared_ptr<Scene> scene, std::string sceneExportPath) {
	scene->serializeJson(sceneExportPath);
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

#ifndef USE_PACKED_ASSETS

void AssetManager::deletePrefabFromDisk(std::string name) {
	assert(prefabAssets.contains(name));
	std::string path = prefabPathsByName.at(name);
	prefabPathsByName.erase(name);
	prefabAssets.erase(name);
	std::filesystem::remove(std::filesystem::path(path));
}
void AssetManager::deleteSceneFromDisk(std::string name) {
	assert(sceneAssets.contains(name));
	std::string path = scenePathsByName.at(name);
	scenePathsByName.erase(name);
	sceneAssets.erase(name);
	std::filesystem::remove(std::filesystem::path(path));
}

#endif

#ifdef USE_PACKED_ASSETS
#else

void AssetManager::createAssetLookups() {

	spritePathsByName.clear();
	spritePathsByID.clear();
	fontPathsByName.clear();
	fontPathsByID.clear();
	prefabPathsByName.clear();
	scenePathsByName.clear();
	//ImagePathsByFileName.clear();

	std::vector<std::string> assetFiles = getAllFilesInDirectory(directories.assetDir);
	std::vector<std::string> prefabFiles = getAllFilesInDirectory(directories.prefabDir);
	std::vector<std::string> sceneFiles = getAllFilesInDirectory(directories.sceneDir);
	//	std::vector<std::string> imageFiles = getAllFilesInDirectory(directories.textureSrcDir);

		// combine all three vectors into one
	assetFiles.insert(assetFiles.end(), prefabFiles.begin(), prefabFiles.end());
	assetFiles.insert(assetFiles.end(), sceneFiles.begin(), sceneFiles.end());
	//		assetFiles.insert(assetFiles.end(), imageFiles.begin(), imageFiles.end());

	for (auto& f : assetFiles) {

		size_t lastindex = f.find_last_of(".");
		std::string extension = f.substr(lastindex, f.length() - 1);


		if (extension == Sprite_extension) {

			Sprite tmpSprite;
			Sprite::deserializeJson(f, &tmpSprite);

			// responsibility of engine to create the default sprite
			if (tmpSprite.ID == defaultSpriteID)
				continue;

			spritePathsByID[tmpSprite.ID] = f;
			spritePathsByName[tmpSprite.name] = f;
		}
		else if (extension == Font_extension) {
			Font tempFont;
			Font::deserializeBinary(f, &tempFont);
			fontPathsByID[tempFont.ID] = f;
			fontPathsByName[tempFont.name] = f;
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