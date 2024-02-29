#pragma once


#include <vector>


#include "Scene.h"
#include "Chest.h"

#include <assetPack/SceneEntities_generated.h>
#include <assetPack/WorldData_generated.h>

class ChunkData {
	
public:
	
	std::vector<Chest> chests;


};

class WorldData {

public:

	WorldData(){}

	std::vector<ChunkData> chunks;

	void LoadAndInstantiateContents(AssetPack::WorldData * worldData) {

		chunks.resize(worldData->chunks()->size());
		for (size_t i = 0; i < chunks.size(); i++)
		{

			ChunkData& chunk = chunks.emplace_back();
			
			auto pack = worldData->chunks()->Get(i);

			for (size_t j = 0; j < pack->chests()->size(); j++)
			{
				chunk.chests.push_back()
			}

		}
	}
};
