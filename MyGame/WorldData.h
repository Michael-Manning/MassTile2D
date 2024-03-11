#pragma once


#include <vector>
#include <utility>
#include <filesystem>

//#include "Scene.h"
#include "MapEntity.h"
#include "Chest.h"

#include "global.h"

#include <assetPack/SceneEntities_generated.h>
#include <assetPack/WorldData_generated.h>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include <robin_hood.h>

#define createAndInitialize(type, initFunc) type(); type.initFunc();

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

		for (size_t i = 0; i < item.size.x; i++)
			for (size_t j = 0; j < item.size.y; j++)
				worldMap->setTile(Blocks::EntityReserve, item.position + glm::ivec2(i, j);
	}
};

class WorldData {

private:

	template<typename T, typename getF>
	void deserializeAdd(ChunkData& chunk, getF getFunc) {
		chunk.getMap<T>().reserve(getFunc->size());
		for (size_t i = 0; i < getFunc->size(); i++)
			chunk.Add<T>(T(getFunc->Get(i)));
	}

	template<typename T, typename PackT>
	auto serializeChunkEntityVector(ChunkData& chunk, flatbuffers::FlatBufferBuilder& builder) {
		std::vector<flatbuffers::Offset<PackT>> entVec;
		auto& srcVec = chunk.getMap<T>();
		entVec.reserve(srcVec.size());
		for (auto& [coordinate, ent] : srcVec)
			entVec.push_back(ent.Serialize(builder));
		return builder.CreateVector(entVec.data(), entVec.size());
	}

public:

	WorldData() {
		chunks.resize(chunkCount);
	}

	std::vector<ChunkData> chunks;

	void LoadAndInstantiateContents() {

	    auto fileSize =	std::filesystem::file_size(std::filesystem::path("myworld.world"));
		std::vector<uint8_t> fileData(static_cast<uint64_t>(fileSize));
		std::ifstream file("myworld.world", std::ios::ate | std::ios::binary);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(fileData.data()), fileData.size());
		file.close();
		const AssetPack::WorldData* worldData = AssetPack::GetWorldData(fileData.data());

		assert(worldData->chunks() != nullptr);

		assert(worldData->chunks()->size() == chunkCount);
		chunks.resize(0);
		chunks.reserve(chunkCount); 

		for (size_t i = 0; i < chunkCount; i++)
		{
			ChunkData& chunk = chunks.emplace_back();

			auto pack = worldData->chunks()->Get(i);

			deserializeAdd<Chest>(chunk, pack->chests());

			//chunk.chests.reserve(pack->chests()->size());
			//for (size_t j = 0; j < chunk.chests.size(); j++)
			//	chunk.Add(Chest(pack->chests()->Get(i)));

		}
	}

	void Serialize() {

		flatbuffers::FlatBufferBuilder builder(1024);

		std::vector<flatbuffers::Offset<AssetPack::ChunkData>> packChunks;

		for (auto& chunk : chunks)
		{
			//auto pack = AssetPack::ChunkDataBuilder(builder);

			//pack.add_chests(serializeChunkEntityVector<Chest, AssetPack::Chest>(chunk, builder));

			auto tmp = AssetPack::CreateChunkData(builder,
				serializeChunkEntityVector<Chest, AssetPack::Chest>(chunk, builder)
			);

			/*std::vector< flatbuffers::Offset<AssetPack::Chest>> chestVec;
			chestVec.reserve(chunk.chests.size());
			for (auto& [coordinate, chest] : chunk.chests)
				chestVec.push_back(chest.Serialize(builder));
			pack.add_chests(builder.CreateVector(chestVec.data(), chestVec.size()));*/


			packChunks.push_back(tmp);
			//packChunks.push_back(pack.Finish());
		}

		auto worldPack = AssetPack::CreateWorldData(builder, builder.CreateVector(packChunks.data(), packChunks.size()));

		builder.Finish(worldPack);

		uint8_t* serializedData = builder.GetBufferPointer();
		auto finalSize = builder.GetSize();

		std::ofstream output("myworld.world", std::ios::binary | std::ios::out);
		output.write(reinterpret_cast<const char*>(serializedData), builder.GetSize());
		output.close();
	}

	template<typename T>
	inline void Add(T&& item) {
		static_assert(std::is_base_of<MapEntity, T>::value, "T must be derived from MapEntity");
		chunks.at(global::tileWorld->GetChunk(item.position)).Add<T>(std::forward<T>(item));
	}
};
