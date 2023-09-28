
#include <vector>
#include <algorithm>
#include <execution>
#include <random>
#include <memory>

#include <glm/glm.hpp>
#include <FastNoise/FastNoise.h>
#include <FastNoise/SmartNode.h>
#include <FastSIMD/FastSIMD.h>

#include "profiling.h"
#include "TileWorld.h"

#include "worldGen.h"


namespace {
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator
	int ran(int min, int max) {
		std::uniform_int_distribution<> dis(min, max);
		return dis(gen);
	}
}

using namespace std;

void WorldGenerator::GenerateTiles(WorldGenSettings& settings) {

	PROFILE_START(World_Gen);

	vector<float> noiseOutput(mapW * mapH);
	vector<float> ironOutput(mapW * mapH);
	vector<bool> blockPresence(mapW * mapH);
	vector<bool> ironPresence(mapW * mapH);

	vector<int> indexes(mapCount - mapPadding);
	std::iota(indexes.begin(), indexes.end(), 0);



	FastNoise::SmartNode<> fnGenerator = FastNoise::NewFromEncodedNodeTree(settings.baseTerrain.nodeTree.c_str(), FastSIMD::CPUMaxSIMDLevel());
	FastNoise::SmartNode<> ironGenerator = FastNoise::NewFromEncodedNodeTree(settings.ironOre.nodeTree.c_str(), FastSIMD::CPUMaxSIMDLevel());
	fnGenerator->GenUniformGrid2D(noiseOutput.data(), -mapW / 2, -mapH / 2, mapW, mapH, settings.baseTerrain.frequency, 1337); // rd()
	ironGenerator->GenUniformGrid2D(ironOutput.data(), -mapW / 2, -mapH / 2, mapW, mapH, settings.ironOre.frequency, 1337);

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
		blockPresence[i] = noiseOutput[i] > settings.baseTerrain.min;
		ironPresence[i] = ironOutput[i] > settings.ironOre.min;
	}





	// upload world data

	auto tworld = world; // encapsolation only works in local scoope?!?
	std::for_each(std::execution::par_unseq, indexes.begin(), indexes.end(), [&tworld, &blockPresence, &ironPresence](const int& i) {

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

		tworld->preloadTile(x, mapH - y - 1, id);
		tworld->preloadBGTile(x, mapH - y - 1, y > (mapH - 205) ? 1023 : 1022);


		});

	PROFILE_END(World_Gen);
}

void WorldGenerator::PostProcess() {

	PROFILE_START(world_post_process);

	//std::for_each(std::execution::seq, indexes.begin(), indexes.end(), [&engine, &blockPresence, &ironPresence](const int& i) {

	for (size_t i = 0; i < mapCount - mapPadding; i++)
	{



		int y = i / mapW;
		int x = i % mapW;

		//if (y < mapH - 3 && y > 3 && x > 3 && x < mapW - 3)
		{
			blockID tileType = world->getTile(x, mapH - y - 1);
			if (tileType != Tiles::Air)
			{

				uint8_t hash = world->getAdjacencyHash(x, mapH - y - 1);
				tileID tile = hash * 3 + 16 * 3 * tileType + ran(0, 2);

				world->preloadTile(x, mapH - y - 1, tile);
			}
			world->preloadBrightness(x, mapH - y - 1, 255 * ambiantLight);
		}

	};

	PROFILE_END(world_post_process);

	world->uploadWorldPreloadData();
}