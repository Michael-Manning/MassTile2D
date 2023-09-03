
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
#include "TileWorld.h"
#include "benchmark.h"
#include "profiling.h"
#include "Utils.h"
#include "worldGen.h"

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



static vec2 GcameraPos;
std::shared_ptr<TileWorld> tileWolrdGlobalRef;

bool showingEditor = false;

#include "tilemapPL.h"

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

	const auto input = engine.GetInput();

	engine.assetManager->loadAllSprites();
	engine.loadPrefabs();

	tileWolrdGlobalRef = engine.worldMap;

	{

		std::ifstream inStream(AssetDirectories.assetDir + "worldgen.json");
		nlohmann::json j;
		inStream >> j;

		auto baseParams = NoiseParams::fromJson(j["baseTerrain"]);
		auto ironParams = NoiseParams::fromJson(j["ironDist"]);


		std::random_device rd; // obtain a random number from hardware


		PROFILE_START(World_Gen);
		vector<float> noiseOutput(mapW * mapH);
		vector<float> ironOutput(mapW * mapH);
		vector<bool> blockPresence(mapW * mapH);
		vector<bool> ironPresence(mapW * mapH);

#ifdef  NDEBUG
		FastNoise::SmartNode<> fnGenerator = FastNoise::NewFromEncodedNodeTree(baseParams.nodeTree.c_str(), FastSIMD::CPUMaxSIMDLevel());
		FastNoise::SmartNode<> ironGenerator = FastNoise::NewFromEncodedNodeTree(ironParams.nodeTree.c_str(), FastSIMD::CPUMaxSIMDLevel());
		fnGenerator->GenUniformGrid2D(noiseOutput.data(), -mapW / 2, -mapH / 2, mapW, mapH, baseParams.frequency, 1337); // rd()
		ironGenerator->GenUniformGrid2D(ironOutput.data(), -mapW / 2, -mapH / 2, mapW, mapH, ironParams.frequency, 1337);

		vector<uint8_t> tData(mapW * mapH * 4);
		
		for (size_t i = 0; i < mapW * mapH; i++) {
			int y = i / mapW;
			int x = i % mapW;

			int j = x + (( mapH - y - 1) * mapW);
			float f = glm::clamp(noiseOutput[j], -1.0f, 1.0f);
			//float f = noiseOutput[j];
			uint8_t val = (f + 1.0f) / 2.0f  * 255;
			tData[i * 4 + 0] = val;
			tData[i * 4 + 1] = val;
			tData[i * 4 + 2] = val;
			tData[i * 4 + 3] = 255;
		}

		for (size_t i = 0; i < mapCount; i++) {
			blockPresence[i] = noiseOutput[i] > baseParams.min;
			ironPresence[i] = ironOutput[i] > ironParams.min;
		}

#endif //  NDEBUG

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

			engine.worldMap->preloadTile(x, mapH - y - 1, id);

			});
		PROFILE_END(world_post_process);

		engine.worldMap->uploadWorldPreloadData();
	}



	engine.setTilemapAtlasTexture(engine.assetManager->spriteAssets[5]->texture);




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
				engine.worldMap->setTile(4, 3, 7);
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
				engine.worldMap->setTile(tileX, mapH - tileY - 1, Tiles::Iron);
			}

		}

		engine.QueueNextFrame();
	}

	engine.Close();

	return 0;
}