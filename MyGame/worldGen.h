#pragma once

#include <string>
#include <memory>

#include <nlohmann/json.hpp>

#include "TileWorld.h"

namespace Blocks {
	constexpr blockID Grass = 0;
	constexpr blockID Dirt = 1;
	constexpr blockID Stone = 2;
	constexpr blockID Iron = 3;
	constexpr tileID Air = 1023;
	constexpr tileID EntityReserve = 1010;
}

constexpr int typesPerTile = 16;
constexpr int tileVariations = 3;
constexpr int tilesPerBlock = typesPerTile * tileVariations;

//static tileID getTileID(blockID block) {
//	switch (block) {
//	case Blocks::Grass:
//		return 0;
//	case Blocks::Dirt:
//		return 1;
//	case Blocks::Stone:
//		return 2;
//	case Blocks::Iron:
//		return 3;
//	default:
//		return 0;
//	}
//}

inline bool IsTransparent(tileID tile) {
	return tile > 1000;
}

inline bool IsSolid(tileID tile) {
	return tile < 1000;
}

inline bool IsMineable(tileID tile) {
	return tile < 1000;
}

struct NoiseParams {
	std::string nodeTree;
	float min;
	float frequency;

	static NoiseParams fromJson(const nlohmann::json& j) {
		NoiseParams p;
		p.nodeTree = j["nodeTree"];
		p.min = j["min"];
		p.min = p.min / 2.0f + 0.5f;
		p.frequency = j["frequency"];
		return p;
	}
};

struct WorldGenSettings {
	NoiseParams baseTerrain;
	NoiseParams ironOre;
};

class WorldGenerator {
public:
	WorldGenerator(TileWorld* worldMap) : world(worldMap) {
	}

	void GenerateTiles(WorldGenSettings& settings);
	void PostProcess();

private:
	TileWorld* world;
};

static constexpr blockID GetBlock(tileID tile) {
	return tile / tilesPerBlock;
}

static constexpr tileID GetFloatingTile(blockID block){
	return block* tilesPerBlock;
}

void CalcTileVariation(uint32_t x, uint32_t y);
inline void CalcTileVariation(glm::ivec2 tile) { return CalcTileVariation(tile.x, tile.y); }

// updates the 9 texture variations around a center tile
void UpdateTextureVariations(glm::ivec2 centerTile);

//static void loadWorld() {
//
//}