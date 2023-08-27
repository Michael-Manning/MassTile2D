#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <set>
#include <optional>
#include <memory>

#include "typedefs.h"

#include "Entity.h"


// Color Renderer
template <>
ColorRenderer* Entity::getComponent() {
	return accessor->_getColorRenderer(ID);
}

// Sprite Renderer
template <>
SpriteRenderer* Entity::getComponent() {
	return accessor->_getSpriteRenderer(ID);
}

// Staticbody
template <>
Staticbody* Entity::getComponent() {
	return accessor->_getStaticbody(ID);
}

// Rigidbody
template <>
Rigidbody* Entity::getComponent() {
	return accessor->_getRigidbody(ID);
}

std::shared_ptr<Input> Entity::input = nullptr;
float Entity::DeltaTime = 0.0f;