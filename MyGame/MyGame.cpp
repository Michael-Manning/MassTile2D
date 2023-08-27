
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
#include <typeinfo>

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

#include "benchmark.h"

const uint32_t winW = 1400;
const uint32_t winH = 800;



using namespace std;
using namespace glm;
using namespace nlohmann;



bool showingEditor = false;

#include "pipelines.h"

const char* shaderPath = "../shaders/compiled/";
//const char* shaderPath = "C:/Users/Michael/source/repos/MyEngine/MyEngine/shaders/compiled/";
//const char* shaderPath = "C:/Users/mmanning/source/repos/MyEngine/MyEngine/shaders/compiled/";

int main() {

	AssetManager::AssetPaths AssetDirectories;
	AssetDirectories.prefabDir = "../data/Prefabs/";
	AssetDirectories.assetDir = "../data/Assets/";
	AssetDirectories.textureSrcDir = "../data/Assets/";
	//AssetDirectories.prefabDir = "C:/Users/Michael/source/repos/MyEngine/MyEngine/data/Prefabs/";
	//AssetDirectories.assetDir = "C:/Users/Michael/source/repos/MyEngine/MyEngine/data/Assets/";
	//AssetDirectories.textureSrcDir = "C:/Users/Michael/source/repos/MyEngine/MyEngine/data/Assets/";
	//AssetDirectories.prefabDir = "C:/Users/mmanning/source/repos/MyEngine/MyEngine/data/Prefabs/";
	//AssetDirectories.assetDir = "C:/Users/mmanning/source/repos/MyEngine/MyEngine/data/Assets/";
	//AssetDirectories.textureSrcDir = "C:/Users/mmanning/source/repos/MyEngine/MyEngine/data/Assets/";


	auto rengine = std::make_shared<VKEngine>();
	Engine engine(rengine, AssetDirectories);
	Editor editor;
	const auto& scene = engine.scene; // quick reference
	engine.Start("video game", winW, winH, shaderPath);




	const auto input = engine.GetInput();

	engine.assetManager->loadAllSprites();
	engine.loadPrefabs();


	// upload world data
	{
		vector<uint32_t> atlasLookup = { 0,3,8,10,14,107,132,137,297, 291,948,749,932, 937, 354 };
		int len = atlasLookup.size() - 1;


		for (size_t i = 0; i < mapCount; i++) {
			//tilemapPipeline.mapData[i] = atlasLookup[ran(0, len)];

			if (i < mapW)
				engine.preloadWorldTile(i % mapW, i / mapW, 930);
			else
				engine.preloadWorldTile(i % mapW, i / mapW, 962);
		}

		engine.uploadWorldPreloadData();
	}

	engine.setAtlasTexture(engine.assetManager->spriteAssets[5]->texture);






	while (!engine.ShouldClose())
	{
		using namespace ImGui;


		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();

		engine.EntityStartUpdate();

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

		if (showingEditor) {
			editor.Run(engine);
		}

		engine.QueueNextFrame();
	}

	engine.Close();

	return 0;
}