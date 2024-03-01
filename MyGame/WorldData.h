#pragma once


#include <vector>
#include <utility>

//#include "Scene.h"
#include "MapEntity.h"
#include "Chest.h"

#include "global.h"

#include <assetPack/SceneEntities_generated.h>
#include <assetPack/WorldData_generated.h>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include <robin_hood.h>


class ChunkData {
	
public:

	robin_hood::unordered_flat_set<MapEntity*> mapEntities;

	template<typename T>
	robin_hood::unordered_node_map <glm::ivec2, T>& getMap();
	
	template<>
	robin_hood::unordered_node_map<glm::ivec2, Chest>& getMap<>() { return chests; }
	robin_hood::unordered_node_map<glm::ivec2, Chest> chests;
	
	template<typename T>
	void Add(T&& item) {
		static_assert(std::is_base_of<MapEntity, T>::value, "T must be derived from MapEntity");
		auto [iter, inserted] = getMap<T>().emplace(item.position, std::move(item));
		mapEntities.insert(&iter->second);
	}
};

class WorldData {

public:

	WorldData(){
		chunks.resize(chunkCount);
	}

	std::vector<ChunkData> chunks;

	void LoadAndInstantiateContents(AssetPack::WorldData * worldData) {

		chunks.resize(worldData->chunks()->size());
		for (size_t i = 0; i < chunks.size(); i++)
		{
			ChunkData& chunk = chunks.emplace_back();
			
			auto pack = worldData->chunks()->Get(i);

			chunk.chests.reserve(pack->chests()->size());
			for (size_t j = 0; j < chunk.chests.size(); j++)
				chunk.Add(Chest(pack->chests()->Get(i)));

		}
	}

	template<typename T>
	inline void Add(T&& item) {
		static_assert(std::is_base_of<MapEntity, T>::value, "T must be derived from MapEntity");
		chunks.at(global::tileWorld->GetChunk(item.position)).Add<T>(std::forward<T>(item));
	}
};
