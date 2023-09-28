
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <execution>
#include <omp.h>
#include<glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <Windows.h>


#include "VKEngine.h"
#include "AssetManager.h"
#include "Engine.h"

#ifdef USING_EDITOR
#include "Editor.h"
#endif 

#include "Physics.h"
#include "Input.h"
#include "BehaviorRegistry.h"
#include "player.h"
#include "demon.h"
#include "TileWorld.h"
#include "benchmark.h"
#include "profiling.h"
#include "Utils.h"
#include "MyMath.h"
#include "worldGen.h"
#include "Settings.h"
#include "BinaryWriter.h"

#include <FastNoise/FastNoise.h>
#include <FastNoise/SmartNode.h>
#include <FastSIMD/FastSIMD.h>

#include <flatbuffers/flatbuffers.h>
#include <assetPack/Sprite_generated.h>

using namespace std;
using namespace glm;
using namespace nlohmann;


std::unordered_map<uint32_t, std::pair<std::string, std::function<std::shared_ptr<Entity>()>>> BehaviorMap = {
	REG_BEHAVIOR(Player),
	REG_BEHAVIOR(Demon)
};



static vec2 GcameraPos;
std::shared_ptr<TileWorld> tileWolrdGlobalRef;

#ifdef USING_EDITOR
bool showingEditor = false;
#else
const bool showingEditor = false;
#endif 


std::random_device rd; // obtain a random number from hardware
std::mt19937 gen(rd()); // seed the generator
int ran(int min, int max) {
	std::uniform_int_distribution<> dis(min, max);
	return dis(gen);
}

void CalcTileVariation(uint32_t x, uint32_t y) {
	if (x > 1 && x < mapW - 1 && y > 1 && y < mapH - 1) {

		tileID curTile = tileWolrdGlobalRef->getTile(x, y) & 0xFFFF;

		if (curTile == Tiles::Air)
			return;

		blockID tileType = curTile / tilesPerBlock;

		uint8_t hash = tileWolrdGlobalRef->getAdjacencyHash(x, y);

		uint32_t curHash = (curTile % tilesPerBlock) / tileVariations;
		if (curHash == hash)
			return;


		tileID tile = hash * tileVariations + tilesPerBlock * tileType + ran(0, 2);
		tileWolrdGlobalRef->setTile(x, y, tile);
	}
}

bool within(vec2 rectStart, vec2  rectEnd, vec2 pos) {
	return pos.x > rectStart.x && pos.x < rectEnd.x && pos.y > rectStart.y && pos.y < rectEnd.y;
}

std::shared_ptr<Input> input = nullptr;
const vec2 btnSize(60, 30);
bool Button(vec2 pos, vec2 size = btnSize) {
	if (input->getMouseBtnDown(MouseBtn::Left) == false)
		return false;
	return within(pos, pos + size, input->getMousePos());
}

#ifdef USING_EDITOR
Editor editor;
#endif 

#ifdef  PUBLISH
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main() {
#endif


	void* resourcePtr = nullptr;
#ifdef USE_EMBEDDED_ASSETS
	{
		HMODULE hModule = GetModuleHandle(NULL);
		LPCSTR lpType = "BINARY";
		LPCSTR lpName = "Assets";
		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCEA(101), lpType);
		if (hResource == NULL) {
			std::cerr << "Unable to find resource." << std::endl;
			return  1;
		}

		// Load the resource
		HGLOBAL hMemory = LoadResource(hModule, hResource);
		if (hMemory == NULL) {
			std::cerr << "Unable to load resource." << std::endl;
			return 1;
		}

		// Lock the resource and get a pointer to the data
		resourcePtr = LockResource(hMemory);
		if (resourcePtr == NULL) {
			std::cerr << "Unable to lock resource." << std::endl;
			return 1;
		}
	}
#endif

	auto exePath = get_executable_directory();
	AssetManager::AssetPaths AssetDirectories;
#ifndef USE_PACKED_ASSETS
	AssetDirectories.prefabDir = makePathAbsolute(exePath, "../../data/Prefabs/") + "/";
	AssetDirectories.assetDir = makePathAbsolute(exePath, "../../data/Assets/") + "/";
	AssetDirectories.fontsDir = makePathAbsolute(exePath, "../../data/Fonts/") + "/";
	AssetDirectories.shaderDir = makePathAbsolute(exePath, "../../shaders/compiled/") + "/";
#endif
	AssetDirectories.resourcePtr = resourcePtr;


	WindowSetting windowSetting;
	windowSetting.windowSizeX = 1400;
	windowSetting.windowSizeY = 800;
	windowSetting.windowMode = WindowMode::Fullscreen;
	windowSetting.name = "video game";

	SwapChainSetting swapchainSettings;
	swapchainSettings.resolutionX = windowSetting.windowSizeX; // not requried in windowed mode
	swapchainSettings.resolutionY = windowSetting.windowSizeY;
	swapchainSettings.vsync = true;
	swapchainSettings.capFramerate = false;

	VideoSettings videoSettings;
	videoSettings.windowSetting = windowSetting;
	videoSettings.swapChainSetting = swapchainSettings;

	auto rengine = std::make_shared<VKEngine>();
	Engine engine(rengine, AssetDirectories);
	const auto& scene = engine.GetCurrentScene(); // quick reference
	engine.Start(videoSettings);

	input = engine.GetInput();

	engine.assetManager->LoadAllSprites();
	engine.assetManager->LoadAllFonts();
	engine.assetManager->LoadAllPrefabs(engine.bworld, false);

	tileWolrdGlobalRef = engine.worldMap;

	{
		// load this from packed resources. 

		//std::ifstream inStream(AssetDirectories.assetDir + "worldgen.json");
		vector<uint8_t> worldGenData;
		engine.assetManager->LoadResourceFile("worldgen.json", worldGenData);
		worldGenData.push_back('\0');
		auto j = nlohmann::json::parse(worldGenData.data());


		auto baseParams = NoiseParams::fromJson(j["baseTerrain"]);
		auto ironParams = NoiseParams::fromJson(j["ironDist"]);





		PROFILE_START(World_Gen);
		vector<float> noiseOutput(mapW * mapH);
		vector<float> ironOutput(mapW * mapH);
		vector<bool> blockPresence(mapW * mapH);
		vector<bool> ironPresence(mapW * mapH);

		vector<int> indexes(mapCount - mapPadding);
		std::iota(indexes.begin(), indexes.end(), 0);

#ifdef  NDEBUG
		FastNoise::SmartNode<> fnGenerator = FastNoise::NewFromEncodedNodeTree(baseParams.nodeTree.c_str(), FastSIMD::CPUMaxSIMDLevel());
		FastNoise::SmartNode<> ironGenerator = FastNoise::NewFromEncodedNodeTree(ironParams.nodeTree.c_str(), FastSIMD::CPUMaxSIMDLevel());
		fnGenerator->GenUniformGrid2D(noiseOutput.data(), -mapW / 2, -mapH / 2, mapW, mapH, baseParams.frequency, 1337); // rd()
		ironGenerator->GenUniformGrid2D(ironOutput.data(), -mapW / 2, -mapH / 2, mapW, mapH, ironParams.frequency, 1337);

		vector<uint8_t> tData(mapW * mapH * 4);

		for (size_t i = 0; i < mapW * mapH; i++) {
			int y = i / mapW;
			int x = i % mapW;

			int j = x + ((mapH - y - 1) * mapW);
			float f = glm::clamp(noiseOutput[j], -1.0f, 1.0f);
			//float f = noiseOutput[j];
			uint8_t val = (f + 1.0f) / 2.0f * 255;
			tData[i * 4 + 0] = val;
			tData[i * 4 + 1] = val;
			tData[i * 4 + 2] = val;
			tData[i * 4 + 3] = 255;
		}

		for (size_t i = 0; i < mapCount - mapPadding; i++) {
			blockPresence[i] = noiseOutput[i] > baseParams.min;
			ironPresence[i] = ironOutput[i] > ironParams.min;
		}



		PROFILE_END(World_Gen);


		// upload world data
		PROFILE_START(world_post_process);


		std::for_each(std::execution::par_unseq, indexes.begin(), indexes.end(), [&engine, &blockPresence, &ironPresence](const int& i) {

			int y = i / mapW;
			int x = i % mapW;

			blockID id = Tiles::Air;

			if ((y < mapH - 1 && y > 1 && x > 1 && x < mapW - 1)) {
				if (blockPresence[y * mapW + x]) {

					id = Tiles::Dirt;

					//if (y < (mapH - 1) && y >(mapH - 205) && blockPresence[(y + 1) * mapW + x] == false) {
					if (y < (mapH - 1) && y >(mapH - 205)) {

						int airCount = 0;
						airCount += blockPresence[(y + 1) * mapW + (x)] == true;
						airCount += blockPresence[(y - 1) * mapW + (x)] == true;
						airCount += blockPresence[(y)*mapW + (x + 1)] == true;
						airCount += blockPresence[(y)*mapW + (x - 1)] == true;

						if (airCount != 4)
							id = Tiles::Grass;
					}
					if (y < mapH - 195) {
						id = Tiles::Stone;

						if (ironPresence[y * mapW + x]) {
							id = Tiles::Iron;
						}
					}
				}
			}

			engine.worldMap->preloadTile(x, mapH - y - 1, id);
			engine.worldMap->preloadBGTile(x, mapH - y - 1, y > (mapH - 205) ? 1023 : 1022);


			});
		//	engine.worldMap->saveToDisk(AssetDirectories.assetDir + "world.dat");
#else
		//engine.worldMap->loadFromDisk(AssetDirectories.assetDir + "world.dat");

		PROFILE_START(world_post_process);
#endif 


		//std::for_each(std::execution::seq, indexes.begin(), indexes.end(), [&engine, &blockPresence, &ironPresence](const int& i) {

		for (size_t i = 0; i < mapCount - mapPadding; i++)
		{



			int y = i / mapW;
			int x = i % mapW;

			//if (y < mapH - 3 && y > 3 && x > 3 && x < mapW - 3)
			{
				blockID tileType = engine.worldMap->getTile(x, mapH - y - 1);
				if (tileType != Tiles::Air)
				{

					uint8_t hash = engine.worldMap->getAdjacencyHash(x, mapH - y - 1);
					tileID tile = hash * 3 + 16 * 3 * tileType + ran(0, 2);

					engine.worldMap->preloadTile(x, mapH - y - 1, tile);
				}
				engine.worldMap->preloadBrightness(x, mapH - y - 1, 255 * ambiantLight);
			}

		};

		PROFILE_END(world_post_process);

		engine.worldMap->uploadWorldPreloadData();
	}

	auto ctest = engine.worldMap->getTile(0, 0);

	//engine.setTilemapAtlasTexture(engine.assetManager->GetSprite("tilemapSprites")->textureID);


	vector<vec2> torchPositions;

	//shared_ptr<Player> player = dynamic_pointer_cast<Player>(scene->Instantiate(engine.assetManager->GetPrefab("Player"), "Player", vec2(0, 106), 0));


	fontID bigfont = engine.assetManager->GetFontID("roboto-32");
	fontID medfont = engine.assetManager->GetFontID("roboto-24");
	fontID smallfont = engine.assetManager->GetFontID("roboto-16");
	//
	float updateTimer = 0;
	float frameRateStat = 0;

	const char* options[] = { "windowed", "windowed fullscreen", "exclusive fullscreen" };
	WindowMode selectedWindowOption = windowSetting.windowMode;
	while (!engine.ShouldClose())
	{
		engine.clearScreenSpaceDrawlist();
		{
			auto white = vec4(1.0);
			//engine.addScreenSpaceQuad(vec4(1.0), input->getMousePos(), vec2(50));

			engine.addScreenSpaceText(smallfont, { 0, 0 }, white, "fps: %d", (int)frameRateStat);

			engine.addScreenSpaceText(bigfont, { 200, 100 }, white, "Settings");

			const auto optionA = "fullscreen";
			engine.addScreenSpaceText(medfont, { 200, 300 }, white, "Window mode: %s", options[(int)selectedWindowOption]);

			engine.addScreenSpaceQuad(white, vec2(900, 320) + btnSize / 2.0f, btnSize);
			if (Button({ 900, 320 })) {
				selectedWindowOption = (WindowMode)(((int)selectedWindowOption + 1) % 3);
			}

			if (engine.time - updateTimer > 0.2f) {
				frameRateStat = engine._getAverageFramerate();
				updateTimer = engine.time;
			}


			//	engine.addScreenSpaceQuad(vec4(0.7), vec2(250, 527), vec2(100, 40));
			vec4 tc = vec4(1.0);
			if (within(vec2(250, 527) - vec2(100, 40) / 2.0f, vec2(250, 527) + vec2(100, 40) / 2.0f, input->getMousePos())) {
				tc = vec4(0.3, 0.9, 0.3, 1.0);
				if (input->getMouseBtnDown(MouseBtn::Left)) {
					videoSettings.windowSetting.windowMode = selectedWindowOption;
					engine.ApplyNewVideoSettings(videoSettings);
				}
			}
			engine.addScreenSpaceText(medfont, { 200, 500 }, tc, "Apply");
		}

		using namespace ImGui;

		GcameraPos = engine.camera.position;

		engine.EntityStartUpdate();

		if (ImGui::GetIO().WantTextInput == false) {
#ifdef USING_EDITOR
			if (input->getKeyDown('e')) {
				showingEditor = !showingEditor;
			}
#endif
			if (input->getKeyDown('p')) {
				engine.paused = !engine.paused;
			}
			if (input->getKeyDown('l')) {
				engine.worldMap->FullLightingUpdate();
			}
			if (input->getKeyDown('m')) {
				break;
			}
		}

#ifdef USING_EDITOR
		if (showingEditor) {
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();

			ImGui::NewFrame();

			editor.editorCamera = engine.camera;
			editor.Run(engine);
			engine.camera = editor.editorCamera;
		}
		else {
			engine.camera.position = GcameraPos;
		}
#else
		engine.camera.position = GcameraPos;
#endif
#if 0
		{

			static int lastX = -1;
			static int lastY = -1;

			vec2 worldClick = engine.screenToWorldPos(input->getMousePos());
			//cout << worldClick.x << " " << worldClick.y << endl;
			int tileX = worldClick.x / tileWorldSize + mapW / 2;
			int tileY = worldClick.y / tileWorldSize + mapH / 2;

			if (tileX > 1 && tileX < mapW - 1 && tileY > 1 && tileY < mapH - 1) {
				int x = tileX;
				int y = mapH - tileY - 1;



				if (input->getMouseBtn(MouseBtn::Left)) {



					/*if (x != lastX || y != lastY) {

						engine.worldMap->setTile(x, y, Tiles::Grass * tilesPerBlock);

						for (int i = -1; i < 2; i++)
							for (int j = -1; j < 2; j++)
								CalcTileVariation(x + i, y + j);

						lastX = x;
						lastY = y;
					}*/

					//if (input->getMouseBtnDown(MouseBtn::Left)) {

					float tileXf = worldClick.x / tileWorldSize + mapW / 2.0f;
					float tileYf = mapH - (worldClick.y / tileWorldSize + mapH / 2.0f) - 1;

					engine.worldMap->setMovingTorch(vec2(tileXf, tileYf), true);


				}
				else {
					//
				//	engine.worldMap->setMovingTorch(ivec2(x, y), false);
				}

				static bool lastState = true;

				if (input->getMouseBtn(MouseBtn::Left) == false && lastState == true) {
					//else if (input->getMouseBtnUp(MouseBtn::Left)) {

				//	torchPositions.push_back(vec2(x, y));


					engine.worldMap->setTorch(x, y);
				}

				lastState = input->getMouseBtn(MouseBtn::Left);

				engine.worldMap->updateLighing();
			}
		}
		if (showingEditor == false && input->getMouseBtn(MouseBtn::Right)) {
			vec2 worldClick = engine.screenToWorldPos(input->getMousePos());

			int tileX = worldClick.x / tileWorldSize + mapW / 2;
			int tileY = worldClick.y / tileWorldSize + mapH / 2;

			if (tileX > 1 && tileX < mapW - 1 && tileY > 1 && tileY < mapH - 1) {

				int x = tileX;
				int y = mapH - tileY - 1;

				engine.worldMap->setTile(x, y, Tiles::Air);

				for (int i = -1; i < 2; i++)
					for (int j = -1; j < 2; j++)
						CalcTileVariation(x + i, y + j);
			}

			/*if (input->getMouseBtnDown(MouseBtn::Right)) {
				auto demon = scene->Instantiate(engine.assetManager.get[["Demon"], "Demon", worldClick, 0);
				dynamic_pointer_cast<Demon>(demon)->setPlayerRef(player);
			}*/
		}
#endif

		engine.QueueNextFrame(showingEditor);
	}

	engine.Close();

	return 0;
}