#include "stdafx.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
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
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <robin_hood.h>
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
#include "TileWorld.h"
#include "profiling.h"
#include "Utils.h"
#include "MyMath.h"
#include "worldGen.h"
#include "Settings.h"
#include "BinaryWriter.h"
#include "worldGen.h"
#include "Menus.h"
#include "GameUI.h"
#include "Behaviour.h"
#include "Benchmarking.h"

#include "player.h"
#include "demon.h"
#include "TestScript.h"

#include <FastNoise/FastNoise.h>
#include <FastNoise/SmartNode.h>
#include <FastSIMD/FastSIMD.h>

#include <flatbuffers/flatbuffers.h>
#include <assetPack/Sprite_generated.h>

using namespace std;
using namespace glm;
using namespace nlohmann;


const BehaviourFactoryMap BehaviorMap = {
	REG_BEHAVIOR(Player),
	REG_BEHAVIOR(Demon),
	REG_BEHAVIOR(TestScript)
};




static vec2 GcameraPos;
TileWorld* tileWolrdGlobalRef = nullptr;

#ifdef USING_EDITOR
bool showingEditor = false;
#else
const bool showingEditor = false;
#endif 

struct Boid {
	vec2 pos;
	vec2 velocity;
	int cellIndex;
};

struct BoidCell {
	int startIndex;
	int endIndex;
};

struct BoidSettings {
	float maxSpeed = 650.0f;
	float visualRange = 60.0f;
	float centeringFactor = 0.64f;
	float minDistance = 11.974f;
	float avoidFactor = 8.397f;
	float matchingFactor = 0.65f;
	float generalAccel = 16.346f;
	float turnFactor = 1000.0f;
	float margin = 150.0f;
	vec2 bounds;
};

BoidSettings boidSettings;

const int boidCount = 3000;

vector<Boid> boids(boidCount);
vector<BoidCell> boidCells;
array<int, boidCount> boidBinIndexes;

namespace {
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator
	int ran(int min, int max) {
		std::uniform_int_distribution<> dis(min, max);
		return dis(gen);
	}

	enum class AppState {
		SplashScreen,
		MainMenu,
		PlayingGame
	};

	AppState appState = AppState::PlayingGame;
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


// globals
std::unique_ptr<Engine> engine = nullptr;
Input* input = nullptr;
shared_ptr<Scene> scene = nullptr;
sceneRenderContextID sceneRenderCtx = 0;
AssetManager::AssetPaths AssetDirectories = {};
TileWorld* worldMap;
Camera mainCamera{
	glm::vec2(0.0f),
	1.0f
};
bool editorToggledThisFrame = false;


const vec2 btnSize(60, 30);
bool Button(vec2 pos, vec2 size = btnSize) {
	if (input->getMouseBtnDown(MouseBtn::Left) == false)
		return false;
	return within(pos, pos + size, input->getMousePos());
}

#ifdef USING_EDITOR
Editor editor;
#endif 

VideoSettings videoSettings = {};

void initializeEngine(std::unique_ptr<Engine>& engine) {

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
#ifndef USE_PACKED_ASSETS
	AssetDirectories.sceneDir = makePathAbsolute(exePath, "../../data/Scenes/") + "/";
	AssetDirectories.prefabDir = makePathAbsolute(exePath, "../../data/Prefabs/") + "/";
	AssetDirectories.assetDir = makePathAbsolute(exePath, "../../data/Assets/") + "/";
	AssetDirectories.fontsDir = makePathAbsolute(exePath, "../../data/Fonts/") + "/";
	AssetDirectories.shaderDir = makePathAbsolute(exePath, "../../shaders/compiled/") + "/";
#endif
	AssetDirectories.resourcePtr = resourcePtr;

	WindowSetting windowSetting;
	windowSetting.windowSizeX = 1400;
	windowSetting.windowSizeY = 800;
	windowSetting.windowMode = WindowMode::Windowed;
	windowSetting.name = "video game";

	SwapChainSetting swapchainSettings;
	swapchainSettings.resolutionX = windowSetting.windowSizeX; // not requried in windowed mode
	swapchainSettings.resolutionY = windowSetting.windowSizeY;
	swapchainSettings.vsync = true;
	swapchainSettings.capFramerate = false;

	videoSettings.windowSetting = windowSetting;
	videoSettings.swapChainSetting = swapchainSettings;

	engine->Start(videoSettings, AssetDirectories);
	input = engine->GetInput();
}

void createTileWorld() {
	{
		// load this from packed resources. 
		worldMap = engine->GetSceneRenderContextTileWorld(sceneRenderCtx);
		WorldGenerator generator(worldMap);
#if NDEBUG
		vector<uint8_t> worldGenData;
		engine->assetManager->LoadResourceFile("worldgen.json", worldGenData);
		worldGenData.push_back('\0');
		auto j = nlohmann::json::parse(worldGenData.data());

		WorldGenSettings settings;
		settings.baseTerrain = NoiseParams::fromJson(j["baseTerrain"]);
		settings.ironOre = NoiseParams::fromJson(j["ironDist"]);

		generator.GenerateTiles(settings);
#else
		worldMap->loadFromDisk(AssetDirectories.assetDir + "world.dat");
#endif
		generator.PostProcess();

	}

	engine->setTilemapAtlasTexture(sceneRenderCtx, engine->assetManager->GetSprite("tilemapSprites")->textureID);

	tileWolrdGlobalRef = worldMap;
}

void queueRenderTasks() {

	if (engine->WindowResizedLastFrame() || (editorToggledThisFrame && showingEditor == false)) {
		engine->ResizeSceneRenderContext(sceneRenderCtx, engine->getWindowSize());
	}

	// draw main scene full screen
	if (showingEditor == false) {
		framebufferID fb = engine->GetSceneRenderContextFramebuffer(sceneRenderCtx);
		engine->addScreenCenteredSpaceFramebufferTexture(fb, engine->getWindowSize() / 2.0f, engine->winH, 0);
	}

	// render main scene no matter what as the editor will use it if active
	Engine::SceneRenderJob mainSceneRender;
	mainSceneRender.scene = scene;
	mainSceneRender.camera = mainCamera;
	mainSceneRender.sceneRenderCtxID = sceneRenderCtx;

#ifdef USING_EDITOR
	if (showingEditor)
		mainSceneRender.camera = editor.editorCamera;
#endif


	vector<Engine::SceneRenderJob> sceneRenderJobs;
	sceneRenderJobs.push_back(mainSceneRender);

	if (showingEditor) {
		auto jobs = editor.GetAdditionalRenderJobs();
		for (auto& job : jobs) {
			sceneRenderJobs.push_back(job);
		}
	}
	engine->QueueNextFrame(sceneRenderJobs, showingEditor);
}

void worldDebug() {

#if 0

	static int lastX = -1;
	static int lastY = -1;

	vec2 worldClick = engine->screenToWorldPos(input->getMousePos());
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
}

constexpr bool useTileWorld = true;

#ifdef  PUBLISH
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main() {
#endif

	// start up engine
	engine = make_unique<Engine>();
	initializeEngine(engine);

	// create or load main scene
	scene = make_shared<Scene>();
	scene->name = "main scene";
	//sceneRenderCtx = engine->CreateSceneRenderContext(engine->getWindowSize(), useTileWorld, vec4(0, 0, 0, 1));
	sceneRenderCtx = engine->CreateSceneRenderContext(engine->getWindowSize(), useTileWorld, { 0.2, 0.3, 1.0, 1 });
#ifdef USING_EDITOR
	editor.Initialize(
		engine.get(), 
		scene, 
		sceneRenderCtx,
		[&](std::shared_ptr<Scene> newScene) {
			scene = newScene; // releases old scene memory
			editor.SetGameScene(newScene);
		}
	);
#endif

	// load all resources 
	engine->assetManager->LoadAllSprites();
	engine->assetManager->LoadAllFonts();
	engine->assetManager->LoadAllPrefabs(false);

	if(useTileWorld)
		createTileWorld();

	vector<vec2> torchPositions;

	//shared_ptr<Player> player = dynamic_pointer_cast<Player>(scene->Instantiate(engine.assetManager->GetPrefab("Player"), "Player", vec2(0, 106), 0));

	UIState UI;
	UI.bigfont = engine->assetManager->GetFontID("roboto-32");
	UI.medfont = engine->assetManager->GetFontID("roboto-24");
	UI.smallfont = engine->assetManager->GetFontID("roboto-16");
	UI.input = input;
	UI.currentPage = UIState::Page::VideoSettings;
	UI.videoSettings = &videoSettings;
	UI.selectedWindowOption = videoSettings.windowSetting.windowMode;

	UI::State uiState;
	uiState.engine = engine.get();
	uiState.selectedHotBarSlot = 0;
	uiState.showingInventory = true;

	{
		//auto testSprite = engine->assetManager->GetSprite("test_cat");
		//Benchmark::BuildSpriteEntityStressTest(scene.get(), testSprite->ID); // 530fps avg
	}

	//auto ent = scene->CreateEntity();
	//scene->DeleteEntity(ent->ID, false);

	while (!engine->ShouldClose())
	{
		ZoneScopedN("main application loop");

		editorToggledThisFrame = false;

		engine->clearScreenSpaceDrawlist();

		if (appState == AppState::MainMenu)
		{
			switch (UI.currentPage)
			{
			case UIState::Page::VideoSettings:
				DoSettingsMenu(UI, engine.get());
				break;
			default:
				break;
			}

		}

		else if (appState == AppState::PlayingGame) {

			engine->addScreenSpaceText(UI.smallfont, { 0, 0 }, vec4(1.0), "fps: %d", (int)engine->_getAverageFramerate());

			//engine.addScreenSpaceTexture("hotbar", 0, vec2(0, 0), 60);
			//UI::DoUI(uiState);

			GcameraPos = mainCamera.position;
			engine->EntityStartUpdate(scene);
			mainCamera.position = GcameraPos;

			if (ImGui::GetIO().WantTextInput == false) {
#ifdef USING_EDITOR
				if (input->getKeyDown('e')) {
					showingEditor = !showingEditor;
					editorToggledThisFrame = true;
				}
#endif
				if (input->getKeyDown('p')) {
					scene->paused = !scene->paused;
				}
				if (input->getKeyDown('l')) {
					worldMap->FullLightingUpdate();
				}
				if (input->getKeyDown('m')) {
					break;
				}
			}

#ifdef USING_EDITOR
			if (showingEditor) {
				ZoneScopedN("Editor");
				ImGui_ImplVulkan_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
				editor.Run();
			}
#endif

			//worldDebug();
		}

		queueRenderTasks();
	}

	engine->Close();

	return 0;
}
