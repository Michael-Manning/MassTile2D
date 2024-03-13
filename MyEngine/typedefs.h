#pragma once

#include <stdint.h>

typedef uint32_t classHash;

typedef int32_t framebufferID;
typedef int32_t sceneGraphicsContextID;
typedef int32_t texID;
typedef uint32_t entityID;
typedef uint32_t spriteID;
typedef uint32_t fontID;
typedef uint32_t atlasID;
typedef uint32_t colliderID;
typedef uint32_t particleSystemID;
typedef uint32_t blockID; // type of block
typedef uint32_t tileID; // specific tile in the texture atlas

constexpr entityID NULL_Entity = 0;

