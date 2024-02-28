#pragma once

#include <glm/glm.hpp>

class MapEntity {
public :

	// anchored top left tile
	glm::ivec2 position;
	glm::ivec2 size; // in tiles

	// decide if I want this class to only support instantiating a prefab, or
	// if I want to support arbitrary entities as well to store extra state?

	// Actually I think this will be a base class to a different category of classes which
	// may include a prefab, but hold there own state which must be individually serialized

	// virtual flatbuffer serialize()
	// virtual deserialize contructor()
};