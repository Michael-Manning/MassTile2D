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
#include "ShaderUtility.h"


#include "VKEngine.h"
#include "AssetManager.h"
#include "Engine.h"

#include "Editor.h"

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
#include "Behaviour.h"
#include "Drawlist.h"
#include "Camera.h"

#include <FastNoise/FastNoise.h>
#include <FastNoise/SmartNode.h>
#include <FastSIMD/FastSIMD.h>

#include <flatbuffers/flatbuffers.h>
#include <assetPack/Sprite_generated.h>

using namespace std;
using namespace glm;
using namespace nlohmann;

static vec2 GcameraPos;
bool showingEditor = false;

// globals
std::unique_ptr<Engine> engine = nullptr;
Input* input = nullptr;
shared_ptr<Scene> scene = nullptr;
sceneGraphicsContextID sceneRenderCtx = 0;
AssetManager::AssetPaths AssetDirectories = {};
unique_ptr<LargeTileWorld> worldMap = nullptr;
Camera mainCamera{
	glm::vec2(-15.0f, 84.0f),
	0.25f
};
bool editorToggledThisFrame = false;


Editor editor;

VideoSettings videoSettings = {};


constexpr int mapw = 32;
constexpr int maph = 32;


const DerivedClassFactoryMap BehaviorMap = {

};

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

	if (showingEditor)
		mainSceneRender.camera = editor.editorCamera;

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
	AssetDirectories.shaderDir = makePathAbsolute(exePath, "../../shaders/compiled/debug") + "/";
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
		//worldMap = engine->GetSceneRenderContextTileWorld(sceneRenderCtx);
		worldMap = engine->CreateLargeTileWorld();
		WorldGenerator generator(worldMap.get());
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

	//global::tileWorld = worldMap.get();
}


glm::vec2 gameSceneWorldToScreenPos(glm::vec2 pos) {

	if (showingEditor)
		return editor.gameSceneWorldToScreenPos(pos);

	return worldToScreenPos(pos, mainCamera, engine->getWindowSize());
}
glm::vec2 gameSceneSreenToWorldPos(glm::vec2 pos) {

	if (showingEditor)
		return editor.gameSceneSreenToWorldPos(pos);

	return screenToWorldPos(pos, mainCamera, engine->getWindowSize());
}

glm::ivec2 GetMouseTile() {
	vec2 mpos = gameSceneSreenToWorldPos(input->getMousePos());
	return worldMap->WorldPosTile(mpos);
}


Scene* mainScene = nullptr;

uint32 waterMassA[mapw * maph];
uint32 waterMassB[mapw * maph];
uint32_t* currentWaterMass = waterMassA;
uint32_t* newWaterMass = waterMassB;
bool waterMassToggle = false;

uint32_t getMass(ivec2 tile) {
	return currentWaterMass[mapw * tile.y + tile.x];
}
void setMass(ivec2 tile, uint32_t mass) {
	newWaterMass[mapw * tile.y + tile.x] = mass;
}

bool isBlock(ivec2 tile) {
	tileID t = worldMap->getTile({ tile.x, LargeTileWorldHeight - (maph - tile.y - 1) - 1 });
	return t != Blocks::Water && t != Blocks::Air;
}

ivec2 up{ 0, -1 };
ivec2 down{ 0, -1 };
ivec2 left{ -1, 0 };
ivec2 right{ 1, 0 };

void simulateWater() {
	
	// sample within padded area
	for (int x = 1; x < mapw - 1; x++)
	{
		for (int y = 1; y < maph - 1; y++)
		{
			ivec2 tile = { x, y };

			/*if (isBlock(tile) == false) {
				setMass(tile, 1);
			}*/

			auto m = getMass(tile);
			
			if (m > 0) {
				if (!isBlock(tile + down)  && getMass(tile + down) == 0) {
					setMass(tile + down, m);
					setMass(tile, 0);
				}
				else {
					setMass(tile, m);
				}
			}

		}
	}

	if (waterMassToggle) {
		currentWaterMass = waterMassA;
		newWaterMass = waterMassB;
	}
	else {
		currentWaterMass = waterMassB;
		newWaterMass = waterMassA;
	}
	waterMassToggle = !waterMassToggle;

	for (int x = 1; x < mapw - 1; x++) {

		for (int y = 1; y < maph - 1; y++) {

			if (isBlock({ x, y }) == false) {
				setMass({x, y}, 0);
				auto m = getMass({ x, y });

				worldMap->setTile({ x, LargeTileWorldHeight - (maph - y - 1) - 1 }, m > 0 ? Blocks::Water : Blocks::Air);
			}
		}
	}
}




int main()
{
	// start up engine
	engine = make_unique<Engine>();
	if (!initializeEngine(engine))
		return 1;


	// create or load main scene
	scene = Scene::MakeScene(engine->assetManager.get()); /*make_shared<Scene>(engine->assetManager.get());*/
	//global::assetManager = engine->assetManager.get();
	//global::input = engine->GetInput();
	//global::SetEngine(engine.get());

	scene->name = "main scene";



	// load all resources 
	engine->assetManager->LoadAllSprites();
	engine->assetManager->LoadAllFonts();
	//engine->assetManager->LoadAllPrefabs(false);

	//engine->assetManager->LoadScene("game_test");
	//scene = engine->assetManager->GetScene("game_test");


	//unique_ptr<WorldData> worldData = nullptr;
	//createTileWorld();
	{
		worldMap = engine->CreateLargeTileWorld();
		WorldGenerator generator(worldMap.get());
	}
	//worldData = make_unique<WorldData>(LargeTileWorld::chunkCount);

	SceneGraphicsAllocationConfiguration sceneConfig;
	sceneConfig.Worldspace_Background_DrawlistLayerCount = 1;
	sceneConfig.Worldspace_Background_LayerAllocations.push_back({});
	sceneConfig.Worldspace_Foreground_DrawlistLayerCount = 1;
	sceneConfig.Worldspace_Foreground_LayerAllocations.push_back({});


	sceneConfig.AllocateLargeTileWorld = true;
	sceneConfig.Framebuffer_ClearColor = { 0.0, 0.0, 0.0, 1 };
	//sceneConfig.Framebuffer_ClearColor = { 0.2, 0.3, 1.0, 1 };

	sceneRenderCtx = engine->CreateSceneRenderContext(engine->getWindowSize(), sceneConfig, worldMap.get());

	//if (useTileWorld) {
	engine->setTilemapAtlasTexture(sceneRenderCtx, engine->assetManager->GetSprite("tilemapSprites")->textureID);

	//}

	worldMap->uploadWorldPreloadData();

	// populate
	{
		for (size_t i = 0; i < LargeTileWorldWidth; i++)
		{
			for (size_t j = 0; j < LargeTileWorldHeight; j++)
			{
				worldMap->setTile(i, j, Blocks::Air);// GetFloatingTile(Blocks::Iron));
			}
		}

		for (size_t i = 0; i < mapw; i++)
		{
			for (size_t j = 0; j < maph; j++)
			{
				if(glm::distance(vec2(i, j), vec2(mapw/2, maph/2)) > 10)
					worldMap->setTile(i, LargeTileWorldHeight - 1 - j, GetFloatingTile(Blocks::Stone));
				//	worldMap->preloadBrightness(i, LargeTileWorldHeight - 1, UINT16_MAX); // ???
			}
		}
		//worldMap->uploadWorldPreloadData();
	}


	mainScene = scene.get();


	editor.Initialize(
		engine.get(),
		scene,
		sceneRenderCtx,
		[&](std::shared_ptr<Scene> newScene) {
			scene = newScene; // releases old scene memory
			editor.SetGameScene(newScene);
			mainScene = newScene.get();
		}
	);
	editor.editorCamera = mainCamera;
	editor.editorCamera.position = vec2(-(LargeTileWorldWidth * tileWorldBlockSize) / 2, (LargeTileWorldHeight * tileWorldBlockSize) / 2);





	//worldMap->FullLightingUpdate();
	//mainCamera.position = vec2(-LargeTileWorldWidth / 2, LargeTileWorldHeight / 2);


	engine->ShowWindow();

	//vector<vec2> torchPositions;


	//auto en = scene->CreateEntity();
	//auto cco = ParticleSystemPL::ParticleSystemConfiguration{};
	//scene->registerComponent(en->ID, ParticleSystemRenderer::Size::Large, cco);


	for (int x = 1; x < mapw - 1; x++)
	{
		for (int y = 1; y < maph - 1; y++)
		{
			currentWaterMass[mapw * y + x] = 0;
			newWaterMass[mapw * y + x] = 0;
		}
	}

	bool firstFrame = true;
	while (!engine->ShouldClose())
	{
		ZoneScopedN("main application loop");

		editorToggledThisFrame = false;

		//engine->clearScreenSpaceDrawlist();


		if (ImGui::GetIO().WantTextInput == false) {
			if (input->getKeyDown('e')) {
				showingEditor = !showingEditor;
				editorToggledThisFrame = true;
			}
			if (input->getKeyDown('p')) {
				scene->paused = !scene->paused;
			}
			//if (input->getKeyDown('l') && useTileWorld) {
			//	worldMap->FullLightingUpdate();
			//}
			if (input->getKeyDown('m')) {
				break;
			}
		}


		auto drawlist = engine->GetScreenspaceDrawlist();


		// pulled out of queue function because it prevents drawing anything on top of the main framebuffer
		{
			if (engine->WindowResizedLastFrame() || (editorToggledThisFrame && showingEditor == false)) {
				engine->ResizeSceneRenderContext(sceneRenderCtx, engine->getWindowSize());
			}

			// draw main scene full screen
			if (showingEditor == false) {



				//if (showLightMapDebug) {
				//	framebufferID fb = engine->_GetSceneRenderContextLightMapBuffer(sceneRenderCtx);
				//	drawlist->AddCenteredFramebufferTexture(fb, engine->getWindowSize() / 2.0f, engine->winH, 0);
				//}
				//else {
				framebufferID fb = engine->GetSceneRenderContextFramebuffer(sceneRenderCtx);
				drawlist->AddCenteredFramebufferTexture(fb, engine->getWindowSize() / 2.0f, engine->winH, 0);
				//}
			}
		}


		//if (useTileWorld) {

		//	// trigger map entity click
		//	{
		//		if (input->getMouseBtnDown(MouseBtn::Right)) {
		//			ivec2 tile = GetMouseTile();
		//			int chunk = worldMap->GetChunk(tile);

		//			for (auto& ent : worldData->chunks[chunk].mapEntities)
		//			{
		//				if (tile.x >= ent->position.x && tile.x < ent->position.x + ent->size.x &&
		//					tile.y >= ent->position.y && tile.y < ent->position.y + ent->size.y) {
		//					ent->OnRightClick();
		//					break; // map entities shouldn't overlap
		//				}
		//			}
		//		}
		//	}
		//}


		GcameraPos = mainCamera.position;
		engine->EntityStartUpdate(scene);
		mainCamera.position = GcameraPos;

		if (showingEditor == false) {
			mainCamera.zoom += input->GetScrollDelta() * 0.1f;
			mainCamera.zoom = glm::clamp(mainCamera.zoom, 0.1f, 10.0f);

		}

		if (showingEditor) {
			ZoneScopedN("Editor");
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			editor.Run();
		}

		//// debug window
		//if (showingEditor && useTileWorld)
		//{
		//	using namespace ImGui;

		//	ivec2 tile = GetMouseTile();
		//	int chunk = worldMap->GetChunk(tile);

		//	if (Begin("Debug")) {

		//		Text("x%d y%d", tile.x, tile.y);
		//		Text("chunk: %d", chunk);
		//		Checkbox("Lighting debug", &showLightMapDebug);
		//		Checkbox("Enable interpolation", reinterpret_cast<bool*>(&worldMap->lightingSettings.interpolationEnabled));
		//		Checkbox("Enable upscale", reinterpret_cast<bool*>(&worldMap->lightingSettings.upscaleEnabled));
		//		Checkbox("Enable blur", reinterpret_cast<bool*>(&worldMap->lightingSettings.blurEnabled));

		//		if (Button("Save world")) {
		//			worldData->Serialize();
		//		}

		//		if (Button("Load world")) {
		//			worldData->LoadAndInstantiateContents();
		//		}

		//		if (input->getKeyDown('n')) {
		//			worldMap->UpdateChunk(chunk);
		//		}


		//		End();
		//	}
		//}




		//if (useTileWorld) {

		//	if (input->getKey('u')) {
		//		vec2 worldClick = gameSceneSreenToWorldPos(input->getMousePos());
		//		float tileXf = worldClick.x / tileWorldBlockSize + mapW / 2.0f;
		//		float tileYf = worldClick.y / tileWorldBlockSize + mapH / 2.0f;

		//		worldMap->setMovingTorch(vec2(tileXf, tileYf), true);
		//	}
		//worldMap->updateLighing();
		//}

		//worldDebug();


		glm::ivec2 tile = GetMouseTile();

		if (tile.x > 1 && tile.x < LargeTileWorldWidth - 1 && tile.y > 1 && tile.y < LargeTileWorldHeight - 1) {


			tileID tileid = worldMap->getTile(tile);

			if (input->getMouseBtn(MouseBtn::Left)) {

				
				worldMap->setTile(tile, Blocks::Air);
				
			}
			else if (input->getKey('c')) {

				//worldMap->setTile(tile, Blocks::Water);
				setMass({ tile.x, maph - (LargeTileWorldHeight - tile.y - 1) - 1 }, 1);
			}
		}

		static float lastUpdate = 0;
		if (engine->time - lastUpdate >= (1.0f / 60.0f)) {
			lastUpdate = engine->time;
			simulateWater();
		}

		//for (int x = 1; x < mapw - 1; x++)
		//{
		//	for (int y = 1; y < maph - 1; y++)
		//	{
		//		if (getMass(x, y) > 0) {
		//			worldMap->setTile()
		//		}
		//		currentWaterMass[mapw * y + x] = 0;
		//		newWaterMass[mapw * y + x] = 0;
		//	}
		//}


		queueRenderTasks();
		firstFrame = false;
	}

	engine->Close();

	return 0;
}