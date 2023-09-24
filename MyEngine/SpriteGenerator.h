#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

#include "typedefs.h"
#include "Sprite.h"

Sprite generateSprite_unidentified(std::string spriteName, FilterMode filterMode) {

	Sprite sprite;
	sprite.name = spriteName;
	sprite.filterMode = filterMode;

	return sprite;
}