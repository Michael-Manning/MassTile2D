#pragma once

//
//constexpr int TilemapPL_MAX_TILES = 1048576 * 8;
//constexpr int largeChunkCount = 131072;
//constexpr int mapW = 1024 * 4;
//constexpr int mapH = 1024 * 2;


constexpr int TilemapPL_MAX_TILES = 1048576 * 2;
constexpr int largeChunkCount = 131072;
constexpr int mapW = 1024 * 2;
constexpr int mapH = 1024 * 1;
constexpr int mapCount = mapW * mapH;

constexpr int chunkSize = 32;
constexpr int chunkTileCount = chunkSize * chunkSize;


constexpr int chunksX = mapW / chunkSize;
constexpr int chunksY = mapH / chunkSize;
constexpr int chunkCount = chunksX * chunksY;

// size of an individual size in wold units
constexpr float tileWorldSize = 0.25f;

static_assert(mapCount == TilemapPL_MAX_TILES);
static_assert(mapW% chunkSize == 0);
static_assert(mapH% chunkSize == 0);
