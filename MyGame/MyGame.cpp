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
#include "Drawlist.h"
#include "Camera.h"

#include "Benchmarking.h"
#include "Collision.h"
#include "Inventory.h"
#include "ItemIDs.h"

#include "player.h"
#include "demon.h"
#include "TestScript.h"
#include "WorldData.h"

#include <FastNoise/FastNoise.h>
#include <FastNoise/SmartNode.h>
#include <FastSIMD/FastSIMD.h>

#include <flatbuffers/flatbuffers.h>
#include <assetPack/Sprite_generated.h>

using namespace std;
using namespace glm;
using namespace nlohmann;


const DerivedClassFactoryMap BehaviorMap = {
	REG_DERIVED(Player),
	REG_DERIVED(Demon),
	REG_DERIVED(TestScript)
};


static vec2 GcameraPos;

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

// globals
std::unique_ptr<Engine> engine = nullptr;
Input* input = nullptr;
shared_ptr<Scene> scene = nullptr;
sceneGraphicsContextID sceneRenderCtx = 0;
AssetManager::AssetPaths AssetDirectories = {};
TileWorld* worldMap;
Camera mainCamera{
	glm::vec2(-15.0f, 84.0f),
	0.25f
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

bool initializeEngine(std::unique_ptr<Engine>& engine) {

	void* resourcePtr = nullptr;
#ifdef USE_EMBEDDED_ASSETS
	{
		HMODULE hModule = GetModuleHandle(NULL);
		LPCSTR lpType = "BINARY";
		LPCSTR lpName = "Assets";
		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCEA(101), lpType);
		if (hResource == NULL) {
			std::cerr << "Unable to find resource." << std::endl;
			return  false;
		}

		// Load the resource
		HGLOBAL hMemory = LoadResource(hModule, hResource);
		if (hMemory == NULL) {
			std::cerr << "Unable to load resource." << std::endl;
			return false;
		}

		// Lock the resource and get a pointer to the data
		resourcePtr = LockResource(hMemory);
		if (resourcePtr == NULL) {
			std::cerr << "Unable to lock resource." << std::endl;
			return false;
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
	windowSetting.windowSizeX = 1900;
	windowSetting.windowSizeY = 1080;
	windowSetting.windowMode = WindowMode::Windowed;
	windowSetting.name = "video game";

	SwapChainSetting swapchainSettings;
	swapchainSettings.resolutionX = windowSetting.windowSizeX; // not requried in windowed mode
	swapchainSettings.resolutionY = windowSetting.windowSizeY;
	swapchainSettings.vsync = true;
	swapchainSettings.capFramerate = false;

	videoSettings.windowSetting = windowSetting;
	videoSettings.swapChainSetting = swapchainSettings;

	DrawlistAllocationConfiguration drawlistMemoryConfig;
	drawlistMemoryConfig.ColoredQuad_MaxInstances = 1024;
	drawlistMemoryConfig.ColoredTriangle_MaxInstances = 64;
	drawlistMemoryConfig.TexturedQuad_MaxInstances = 1024;
	drawlistMemoryConfig.Text_MaxStringLength = 128;
	drawlistMemoryConfig.Text_MaxStrings = 100;

	EngineMemoryAllocationConfiguration engineMemoryConfig;
	engineMemoryConfig.ParticleSystem_MaxLargeSystems = 4;
	engineMemoryConfig.ParticleSystem_MaxLargeSystemParticles = 100000;
	engineMemoryConfig.Screenspace_DrawlistLayerCount = 1;
	engineMemoryConfig.Screenspace_DrawlistLayerAllocations.push_back(drawlistMemoryConfig);


	engine->Start(videoSettings, AssetDirectories, engineMemoryConfig);
	input = engine->GetInput();

	return true;
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
		worldMap->saveToDisk(AssetDirectories.assetDir + "world.dat");
#else
		worldMap->loadFromDisk(AssetDirectories.assetDir + "world.dat");
#endif
		generator.PostProcess();

	}

	engine->setTilemapAtlasTexture(sceneRenderCtx, engine->assetManager->GetSprite("tilemapSprites")->textureID);

	worldMap->FullLightingUpdate();

	global::tileWorld = worldMap;
}

void queueRenderTasks() {

	//if (engine->WindowResizedLastFrame() || (editorToggledThisFrame && showingEditor == false)) {
	//	engine->ResizeSceneRenderContext(sceneRenderCtx, engine->getWindowSize());
	//}

	//// draw main scene full screen
	//if (showingEditor == false) {
	//	framebufferID fb = engine->GetSceneRenderContextFramebuffer(sceneRenderCtx);
	//	engine->addScreenCenteredSpaceFramebufferTexture(fb, engine->getWindowSize() / 2.0f, engine->winH, 0);
	//}

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

#ifdef USING_EDITOR
	if (showingEditor) {
		auto jobs = editor.GetAdditionalRenderJobs();
		for (auto& job : jobs) {
			sceneRenderJobs.push_back(job);
		}
	}
#endif

	engine->QueueNextFrame(sceneRenderJobs, showingEditor);
}

glm::ivec2 GetMouseTile() {
	vec2 mpos = gameSceneSreenToWorldPos(input->getMousePos());
	return worldMap->WorldPosTile(mpos);
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


constexpr bool useTileWorld = false;
bool showLightMapDebug = false;

#ifdef  PUBLISH
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main() {
#endif

	// start up engine
	engine = make_unique<Engine>();
	if (!initializeEngine(engine))
		return 1;

	// create or load main scene
	scene = Scene::MakeScene(engine->assetManager.get()); /*make_shared<Scene>(engine->assetManager.get());*/
	global::assetManager = engine->assetManager.get();
	global::input = engine->GetInput();
	global::SetEngine(engine.get());

	scene->name = "main scene";
	//sceneRenderCtx = engine->CreateSceneRenderContext(engine->getWindowSize(), useTileWorld, vec4(0, 0, 0, 1));
	//sceneRenderCtx = engine->CreateSceneRenderContext(engine->getWindowSize(), useTileWorld, { 0.2, 0.3, 1.0, 1 });

	SceneGraphicsAllocationConfiguration sceneConfig;
	sceneConfig.AllocateTileWorld = useTileWorld;
	sceneConfig.Framebuffer_ClearColor = { 0.0, 0.0, 0.0, 1 };
	//sceneConfig.Framebuffer_ClearColor = { 0.2, 0.3, 1.0, 1 };

	sceneRenderCtx = engine->CreateSceneRenderContext(engine->getWindowSize(), sceneConfig);


	// load all resources 
	engine->assetManager->LoadAllSprites();
	engine->assetManager->LoadAllFonts();
	engine->assetManager->LoadAllPrefabs(false);

	if (useTileWorld) {
		engine->assetManager->LoadScene("game_test");
		scene = engine->assetManager->GetScene("game_test");
	}

	global::mainScene = scene.get();

#ifdef USING_EDITOR
	editor.Initialize(
		engine.get(),
		scene,
		sceneRenderCtx,
		[&](std::shared_ptr<Scene> newScene) {
			scene = newScene; // releases old scene memory
			editor.SetGameScene(newScene);
			global::mainScene = newScene.get();
		}
	);
	editor.editorCamera = mainCamera;
#endif



	unique_ptr<WorldData> worldData = nullptr;

	if (useTileWorld) {
		createTileWorld();
		worldData = make_unique<WorldData>();
	}

	//const auto& myTest = worldData->chunks[0].chests.at(glm::ivec2(6, 8));

	engine->ShowWindow();

	vector<vec2> torchPositions;

	//shared_ptr<Player> player = dynamic_pointer_cast<Player>(scene->Instantiate(engine.assetManager->GetPrefab("Player"), "Player", vec2(0, 106), 0));

	MenuState UI;
	UI.bigfont = engine->assetManager->GetFontID("roboto-32");
	UI.medfont = engine->assetManager->GetFontID("roboto-24");
	UI.smallfont = engine->assetManager->GetFontID("roboto-16");
	UI.input = input;
	UI.currentPage = MenuState::Page::VideoSettings;
	UI.videoSettings = &videoSettings;
	UI.selectedWindowOption = videoSettings.windowSetting.windowMode;

	UI::State uiState;
	uiState.engine = engine.get();
	uiState.selectedHotBarSlot = 0;
	uiState.showingInventory = true;
	uiState.bigfont = engine->assetManager->GetFontID("roboto-32");
	uiState.medfont = engine->assetManager->GetFontID("roboto-24");
	uiState.smallfont = engine->assetManager->GetFontID("roboto-16");

	{
		//auto testSprite = engine->assetManager->GetSprite("test_cat");
		//Benchmark::BuildSpriteEntityStressTest(scene.get(), testSprite->ID); // 300 avg 2400 release
		// 460, 2800
	}


	if (useTileWorld) {
		global::playerInventory.slots[4] = ItemStack(
			Sword_ItemID,
			1
		);
		global::playerInventory.slots[3] = ItemStack(
			Pickaxe_ItemID,
			1
		);
		global::playerInventory.slots[15] = ItemStack(
			Bow_ItemID,
			1
		);
		global::playerInventory.slots[16] = ItemStack(
			Apple_ItemID,
			40
		);
		global::playerInventory.slots[33] = ItemStack(
			Apple_ItemID,
			9
		);
		global::playerInventory.slots[34] = ItemStack(
			Apple_ItemID,
			49
		);

		global::playerInventory.slots[36] = ItemStack(
			Chest_ItemID,
			1
		);
	}

	auto en = scene->CreateEntity();
	auto cco = ParticleSystemPL::ParticleSystemConfiguration{};
	scene->registerComponent(en->ID, ParticleSystemRenderer::ParticleSystemSize::Large, cco);


	bool firstFrame = true;
	while (!engine->ShouldClose())
	{
		ZoneScopedN("main application loop");

		editorToggledThisFrame = false;

		//engine->clearScreenSpaceDrawlist();


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
			if (input->getKeyDown('l') && useTileWorld) {
				worldMap->FullLightingUpdate();
			}
			if (input->getKeyDown('m')) {
				break;
			}
		}



		// pulled out of queue function because it prevents drawing anything on top of the main framebuffer
		{
			if (engine->WindowResizedLastFrame() || (editorToggledThisFrame && showingEditor == false)) {
				engine->ResizeSceneRenderContext(sceneRenderCtx, engine->getWindowSize());
			}

			// draw main scene full screen
			if (showingEditor == false) {

				auto drawlist = engine->GetScreenspaceDrawlist();

				if (showLightMapDebug) {
					framebufferID fb = engine->_GetSceneRenderContextLightMapBuffer(sceneRenderCtx);
					drawlist->AddCenteredFramebufferTexture(fb, engine->getWindowSize() / 2.0f, engine->winH, 0);
				}
				else {
					framebufferID fb = engine->GetSceneRenderContextFramebuffer(sceneRenderCtx);
					drawlist->AddCenteredFramebufferTexture(fb, engine->getWindowSize() / 2.0f, engine->winH, 0);
				}
			}
		}


		if (appState == AppState::MainMenu)
		{
			switch (UI.currentPage)
			{
			case MenuState::Page::VideoSettings:
				DoSettingsMenu(UI, engine.get());
				break;
			default:
				break;
			}

		}

		else if (appState == AppState::PlayingGame) {

			auto drawlist = engine->GetScreenspaceDrawlist();

			drawlist->AddText(UI.smallfont, { 4, 4 }, vec4(1.0), "fps: %d", (int)engine->_getAverageFramerate());


			if (useTileWorld) {
				UI::DoUI(uiState);



				// trigger map entity click
				{
					if (input->getMouseBtnDown(MouseBtn::Right)) {
						ivec2 tile = GetMouseTile();
						int chunk = worldMap->GetChunk(tile);

						for (auto& ent : worldData->chunks[chunk].mapEntities)
						{
							if (tile.x >= ent->position.x && tile.x < ent->position.x + ent->size.x &&
								tile.y >= ent->position.y && tile.y < ent->position.y + ent->size.y) {
								ent->OnRightClick();
								break; // map entities shouldn't overlap
							}
						}
					}
				}
			}


			GcameraPos = mainCamera.position;
			engine->EntityStartUpdate(scene);
			mainCamera.position = GcameraPos;

			if (showingEditor == false) {
				mainCamera.zoom += input->GetScrollDelta() * 0.1f;
				mainCamera.zoom = glm::clamp(mainCamera.zoom, 0.1f, 10.0f);

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

			// debug window
			if (showingEditor && useTileWorld)
			{
				using namespace ImGui;

				ivec2 tile = GetMouseTile();
				int chunk = worldMap->GetChunk(tile);

				if (Begin("Debug")) {

					Text("x%d y%d", tile.x, tile.y);
					Text("chunk: %d", chunk);
					Checkbox("Lighting debug", &showLightMapDebug);
					Checkbox("Enable interpolation", reinterpret_cast<bool*>(&worldMap->lightingSettings.interpolationEnabled));
					Checkbox("Enable upscale", reinterpret_cast<bool*>(&worldMap->lightingSettings.upscaleEnabled));
					Checkbox("Enable blur", reinterpret_cast<bool*>(&worldMap->lightingSettings.blurEnabled));

					if (Button("Save world")) {
						worldData->Serialize();
					}

					if (Button("Load world")) {
						worldData->LoadAndInstantiateContents();
					}

					if (input->getKeyDown('n')) {
						worldMap->UpdateChunk(chunk);
					}


					End();
				}
			}



			//vector<vec4> debugColors;

			//for (size_t i = 0; i < debugTriangles.size() / 3; i++)
			//{
			//	debugColors.push_back(vec4(1.0, 0.0, 0.0, 0.5));
			//}

			//engine->SceneTriangles(sceneRenderCtx, debugTriangles, debugColors);

			//debugTriangles.clear();


			if (useTileWorld) {

				if (input->getKey('u')) {
					vec2 worldClick = gameSceneSreenToWorldPos(input->getMousePos());
					float tileXf = worldClick.x / tileWorldSize + mapW / 2.0f;
					float tileYf = worldClick.y / tileWorldSize + mapH / 2.0f;

					worldMap->setMovingTorch(vec2(tileXf, tileYf), true);
				}
				worldMap->updateLighing();
			}

			//worldDebug();
		}

		queueRenderTasks();
		firstFrame = false;
	}

	engine->Close();

	return 0;
}
