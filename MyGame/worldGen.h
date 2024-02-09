#pragma once

#include <string>
#include <memory>

#include <nlohmann/json.hpp>

#include "TileWorld.h"

namespace Tiles {
	constexpr blockID Grass = 0;
	constexpr blockID Dirt = 1;
	constexpr blockID Stone = 2;
	constexpr blockID Iron = 3;
	constexpr blockID Air = 1023;
}

constexpr int typesPerTile = 16;
constexpr int tileVariations = 3;
constexpr int tilesPerBlock = typesPerTile * tileVariations;

static tileID getTileID(blockID block) {
	switch (block) {
	case Tiles::Grass:
		return 0;
	case Tiles::Dirt:
		return 1;
	case Tiles::Stone:
		return 2;
	case Tiles::Iron:
		return 3;
	default:
		return 0;
	}
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


static void loadWorld() {

}