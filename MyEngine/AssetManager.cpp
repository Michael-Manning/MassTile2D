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

texID AssetManager::loadTexture(int w, int h, std::vector<uint8_t>& data, FilterMode filterMode, bool imGuiTexure, texID inputID) {

	Texture tex = rengine->genTexture(w, h, data, filterMode);

	texID id;

	if (inputID != -1) {
		id = TextureIDGenerator.GenerateID();
	}
	else {
		id = inputID;
		TextureIDGenerator.Input(inputID);
	}

	if (imGuiTexure) {
		tex.imTexture = ImGui_ImplVulkan_AddTexture(tex.sampler, tex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	textureAssets[id] = tex;
	imageSources[id] = "N/A";

	return id;
}

texID AssetManager::loadTexture(std::string imagePath, FilterMode filterMode, bool imGuiTexure) {

	std::string filename = std::filesystem::path(imagePath).filename().string();

	// could make debug only check
	texID res = TextureIDGenerator.ContainsHash(filename);
	if (res) {
		return res;
	}

	Texture tex = rengine->genTexture(imagePath, filterMode);

	//texID id = TextureIDGenerator.GenerateID();

	texID id = TextureIDGenerator.GenerateID(filename);

	if (imGuiTexure) {
		tex.imTexture = ImGui_ImplVulkan_AddTexture(tex.sampler, tex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	textureAssets[id] = tex;
	imageSources[id] = imagePath;

	return id;
}

void AssetManager::updateTexture(texID id, FilterMode filterMode) {
	auto& tex = textureAssets[id];
	tex.sampler = (filterMode == FilterMode::Nearest) ? rengine->textureSampler_nearest : rengine->textureSampler_linear;
	/*spritesAdded = true;*/
	filterModesChanged = true;
}

void AssetManager::loadPrefabs(std::shared_ptr<b2World> world) {
	auto files = getAllFilesInDirectory(std::filesystem::path(directories.prefabDir));

	prefabs.clear();

	for (auto& i : files)
	{
		std::string name = std::filesystem::path(i).filename().string();
		size_t lastindex = name.find_last_of(".");
		std::string rawname = name.substr(0, lastindex);

		std::string extension = name.substr(lastindex, name.length() - 1);
		if (extension != Prefab_extension)
			continue;

		prefabs[rawname] = Prefab::deserializeJson(i, world);
	}
}


// loads sprite assets
void AssetManager::loadSpriteAssets(std::set<spriteID> _ids) {
	auto spriteFiles = getAllFilesInDirectory(std::filesystem::path(directories.assetDir));
	auto imageFiles = getAllFilesInDirectory(std::filesystem::path(directories.textureSrcDir));

	// only load unloaded ids
	std::set<spriteID> ids;
	for (auto& i : _ids) {
		if (spriteAssets.contains(i) == false)
			ids.insert(i);
	}

	vector<pair<string, FilterMode>> requiredTextureFiles;
	vector <shared_ptr<Sprite>> loadedSprites;

	for (auto& i : spriteFiles)
	{
		std::string name = std::filesystem::path(i).filename().string();
		size_t lastindex = name.find_last_of(".");

		std::string extension = name.substr(lastindex, name.length() - 1);

		if (extension != Sprite_extension)
			continue;

		auto sprite = Sprite::deserializeJson(i);

		if (ids.contains(sprite->ID) == false)
			continue;

		loadedSprites.push_back(sprite);

		if (textureAssets.contains(sprite->texture) == false)
			requiredTextureFiles.push_back({ sprite->fileName, sprite->filterMode });
	}

	if (loadedSprites.size() != ids.size())
		throw new std::exception("Could not locate all required sprites");

	for (auto& i : loadedSprites) {
		spriteAssets[i->ID] = i;
		SpriteGenerator.Input(i->ID);
	}

	set<string> imageFileNames;
	for (auto& i : imageFiles) {
		std::string name = std::filesystem::path(i).filename().string();
		imageFileNames.insert(name);
	}

	for (auto& i : requiredTextureFiles)
	{
		if (imageFileNames.contains(i.first) == false)
			throw new std::exception("could not find required texture file");

		loadTexture(directories.textureSrcDir + i.first, i.second);
	}

}

void AssetManager::loadFontAssets(std::set<fontID> _ids) {
	auto fontFiles = getAllFilesInDirectory(std::filesystem::path(directories.assetDir));

	// only load unloaded ids
	std::set<fontID> ids;
	for (auto& i : _ids) {
		if (fontAssets.contains(i) == false)
			ids.insert(i);
	}

	set<spriteID> requiredSpriteIDs;
	vector <shared_ptr<Font>> loadedFonts;

	for (auto& i : fontFiles)
	{
		std::string name = std::filesystem::path(i).filename().string();
		size_t lastindex = name.find_last_of(".");

		std::string extension = name.substr(lastindex, name.length() - 1);

		if (extension != Font_extension)
			continue;

		auto font = Font::deserializeJson(i);

		if (ids.contains(font->ID) == false)
			continue;

		loadedFonts.push_back(font);
		requiredSpriteIDs.insert(font->atlas);
	}

	if (loadedFonts.size() != ids.size())
		throw new std::exception("Could not locate all required fonts");

	for (auto& i : loadedFonts) {
		fontAssets[i->ID] = i;
		fontGenerator.Input(i->ID);
	}

	loadSpriteAssets(requiredSpriteIDs);
}

void AssetManager::loadAllSprites() {

	assert(allLoaded == false);
	allLoaded = true;

	auto spriteFiles = getAllFilesInDirectory(std::filesystem::path(directories.assetDir));
	auto imageFiles = getAllFilesInDirectory(std::filesystem::path(directories.textureSrcDir));


	vector<pair<string, FilterMode>> requiredTextureFiles;
	vector < shared_ptr<Sprite>> loadedSprites;

	for (auto& i : spriteFiles)
	{
		std::string name = std::filesystem::path(i).filename().string();
		size_t lastindex = name.find_last_of(".");

		std::string extension = name.substr(lastindex, name.length() - 1);

		if (extension != Sprite_extension)
			continue;

		auto sprite = Sprite::deserializeJson(i);

		loadedSprites.push_back(sprite);

		if (textureAssets.contains(sprite->texture) == false)
			requiredTextureFiles.push_back({ sprite->fileName, sprite->filterMode });
	}

	for (auto& i : loadedSprites) {
		spriteAssets[i->ID] = i;
		SpriteGenerator.Input(i->ID);
	}

	set<string> imageFileNames;
	for (auto& i : imageFiles) {
		std::string name = std::filesystem::path(i).filename().string();
		imageFileNames.insert(name);
	}

	for (auto& i : requiredTextureFiles)
	{
		if (imageFileNames.contains(i.first) == false)
			throw new std::exception("could not find required texture file");

		loadTexture(directories.textureSrcDir + i.first, i.second);
	}
}

shared_ptr<Sprite> AssetManager::GenerateSprite(std::string imagePath, FilterMode filterMode, bool genImgui) {

	texID id = loadTexture(imagePath, filterMode, genImgui);
	const auto& tex = textureAssets[id];

	std::string filename = std::filesystem::path(imagePath).filename().string();

	auto sprite = make_shared<Sprite>();
	sprite->texture = id;
	sprite->fileName = filename;
	sprite->ID = SpriteGenerator.GenerateID();
	sprite->resolution = glm::vec2(tex.resolutionX, tex.resolutionY);

	spritesAdded = true;

	spriteAssets[sprite->ID] = sprite;


	return sprite;
}

void AssetManager::CreateDefaultSprite(int w, int h, std::vector<uint8_t>& data) {

	texID id = loadTexture(w, h, data, FilterMode::Nearest, true, 0);
	const auto& tex = textureAssets[id];

	auto sprite = make_shared<Sprite>();
	sprite->texture = id;
	sprite->ID = SpriteGenerator.GenerateID();
	sprite->resolution = glm::vec2(tex.resolutionX, tex.resolutionY);

	spritesAdded = true;

	defaultSprite = sprite->ID;
	spriteAssets[sprite->ID] = sprite;
}



void AssetManager::addFont(Font font) {
	fontID id = fontGenerator.GenerateID();
	fontAssets[id] = make_shared<Font>(font);
}

void AssetManager::addFont(Font font, fontID inputID) {
	fontGenerator.Input(inputID);
	fontAssets[inputID] = make_shared<Font>(font);
}





















