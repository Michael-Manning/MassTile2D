#include "stdafx.h"

#include <stdint.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <set>
#include <optional>
#include <memory>

#include "typedefs.h"

#include "Entity.h"

using namespace glm;

// TODO create own transform functions for 2D (for performance)

void Entity::localTransformRecursive(glm::mat4* m) const {
	if (parent != nullptr)
		parent->localTransformRecursive(m);

	*m = translate(*m, vec3(transform.position, 0.0f));
	*m = rotate(*m, transform.rotation, vec3(0.0f, 0.0f, 1.0f));
}

glm::mat4 Entity::GetLocalToGlobalMatrix() const {
	assert(HasParent()); // speeddddd

	mat4 m(1.0f);
	parent->localTransformRecursive(&m);
	return m;
}

Transform Entity::GetGlobalTransform() const {

	if (HasParent() == false)
		return transform;

	mat4 m = GetLocalToGlobalMatrix();
	m = translate(m, vec3(transform.position, 0.0f));
	m = rotate(m, transform.rotation, vec3(0.0f, 0.0f, 1.0f));

	Transform t(
		extractPosition(m),
		transform.scale,
		extractRotation(m)
	);

	return t;
}

void Entity::globalTransformRecursive(glm::mat4* m) const {

	*m = rotate(*m, -transform.rotation, vec3(0.0f, 0.0f, 1.0f));
	*m = translate(*m, vec3(-transform.position, 0.0f));

	if (parent != nullptr)
		parent->localTransformRecursive(m);
}

glm::mat4 Entity::GetGlobalToLocalMatrix() const {
	assert(HasParent()); // speeddddd

	mat4 m(1.0f);
	parent->globalTransformRecursive(&m);
	return m;
}

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