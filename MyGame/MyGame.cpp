
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

#include "VKEngine.h"
#include "Engine.h"
#include "Physics.h"

#include<glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <imgui.h>

#include "Editor.h"
#include "MyMath.h"

#include "BehaviorRegistry.h"
#include "ball.h"
#include "player.h"

#include "benchmark.h"
#include "profiling.h"
#include "Utils.h"

#include <FastNoise/FastNoise.h>
#include <FastNoise/SmartNode.h>
#include <FastSIMD/FastSIMD.h>


const uint32_t winW = 1400;
const uint32_t winH = 800;



using namespace std;
using namespace glm;
using namespace nlohmann;




std::unordered_map<uint32_t, std::pair<std::string, std::function<std::shared_ptr<Entity>()>>> BehaviorMap = {
	REG_BEHAVIOR(Ball),
	REG_BEHAVIOR(Player),
};


// temporary until I deside how to make this data accessible through a tilemap renderer
static TilemapPL* tileWolrdGlobalRef;
static vec2 GcameraPos;

bool showingEditor = false;

#include "pipelines.h"

int main() {

	auto exePath = get_executable_directory();

	string shaderPath = makePathAbsolute(exePath, "../../shaders/compiled/") + "/";
	AssetManager::AssetPaths AssetDirectories;
	AssetDirectories.prefabDir = makePathAbsolute(exePath, "../../data/Prefabs/") + "/";
	AssetDirectories.assetDir = makePathAbsolute(exePath, "../../data/Assets/") + "/";
	AssetDirectories.textureSrcDir = makePathAbsolute(exePath, "../../data/Assets/") + "/";



	auto rengine = std::make_shared<VKEngine>();
	Engine engine(rengine, AssetDirectories);
	Editor editor;
	const auto& scene = engine.scene; // quick reference
	engine.Start("video game", winW, winH, shaderPath);


	tileWolrdGlobalRef = &engine.tilemapPipeline;

	const auto input = engine.GetInput();

	engine.assetManager->loadAllSprites();
	engine.loadPrefabs();




	{
		string baseTerrainGen;
		string ironDist;
		{
			std::ifstream input(AssetDirectories.assetDir + "worldgen.json");
			nlohmann::json j;
			input >> j;
			baseTerrainGen = j["baseTerrain"];
			ironDist = j["ironDist"];
		}

		std::random_device rd; // obtain a random number from hardware


		PROFILE_START(World_Gen);
		vector<float> noiseOutput(mapW * mapH);
		vector<float> ironOutput(mapW * mapH);
		vector<bool> blockPresence(mapW * mapH);
		vector<bool> ironPresence(mapW * mapH);

		FastNoise::SmartNode<> fnGenerator = FastNoise::NewFromEncodedNodeTree(baseTerrainGen.c_str(), FastSIMD::CPUMaxSIMDLevel());
		FastNoise::SmartNode<> ironGenerator = FastNoise::NewFromEncodedNodeTree(ironDist.c_str(), FastSIMD::CPUMaxSIMDLevel());
		fnGenerator->GenUniformGrid2D(noiseOutput.data(), 0, -mapH + 100, mapW, mapH, 0.032f, rd());
		ironGenerator->GenUniformGrid2D(ironOutput.data(), 0, 0, mapW, mapH, 0.003f, rd());


		for (size_t i = 0; i < mapCount; i++) {
			blockPresence[i] = noiseOutput[i] > 0.4f;
			ironPresence[i] = ironOutput[i] > 0.6f;
		}
		PROFILE_END(World_Gen);


		// upload world data
		PROFILE_START(world_post_process);
		vector<int> indexes(mapCount);
		std::iota(indexes.begin(), indexes.end(), 0);

		std::for_each(std::execution::par_unseq, indexes.begin(), indexes.end(), [&engine, &blockPresence, &ironPresence](const int& i) {

			int y = i / mapW;
			int x = i % mapW;

			blockID id = Tiles::Air;
			if (blockPresence[y * mapW + x]) {
				id = Tiles::Dirt;

				if (y < (mapH - 1) && y >(mapH - 205) && blockPresence[(y + 1) * mapW + x] == false) {
					id = Tiles::Grass;
				}
				if (y < mapH - 195) {
					id = Tiles::Stone;

					if (ironPresence[y * mapW + x]) {
						id = Tiles::Iron;
					}
				}
			}

			engine.preloadWorldTile(x, mapH - y - 1, id);

			});
		PROFILE_END(world_post_process);

		engine.uploadWorldPreloadData();
	}



	engine.setAtlasTexture(engine.assetManager->spriteAssets[5]->texture);




	while (!engine.ShouldClose())
	{
		using namespace ImGui;


		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();

		if (ImGui::GetIO().WantTextInput == false) {
			if (input->getKeyDown('e')) {
				showingEditor = !showingEditor;
			}
			if (input->getKeyDown('p')) {
				engine.paused = !engine.paused;
			}
			if (input->getKeyDown('t')) {
				engine.setWorldTile(4, 3, 7);
			}
		}

		GcameraPos = engine.camera.position;

		engine.EntityStartUpdate();

		if (showingEditor == false)
			engine.camera.position = GcameraPos;

		if (showingEditor) {
			editor.editorCamera = engine.camera;
			editor.Run(engine);
			engine.camera = editor.editorCamera;
		}

		if (input->getMouseBtn(MouseBtn::Left)) {
			vec2 worldClick = engine.screenToWorldPos(input->getMousePos());

			int tileX = worldClick.x / tileWorldSize + mapW / 2;
			int tileY = worldClick.y / tileWorldSize + mapH / 2;

			if (tileX > 0 && tileX < mapW && tileY > 0 && tileY < mapH) {
				engine.setWorldTile(tileX, mapH - tileY - 1, Tiles::Iron);
			}

		}

		engine.QueueNextFrame();
	}

	engine.Close();

	return 0;
}