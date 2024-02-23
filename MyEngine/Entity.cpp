#include "stdafx.h"

#include <stdint.h>
#include <string>
#include <set>
#include <optional>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include <assetPack/common_generated.h>

#include "typedefs.h"

#include "Entity.h"

using namespace glm;

// TODO create own transform functions for 2D (for performance)

void Entity::localTransformRecursive(glm::mat4* m) const {
	if (parent_cachePtr != nullptr)
		parent_cachePtr->localTransformRecursive(m);

	*m = translate(*m, vec3(transform.position, 0.0f));
	*m = rotate(*m, transform.rotation, vec3(0.0f, 0.0f, 1.0f));
}

glm::mat4 Entity::GetLocalToGlobalMatrix() const {
	assert(HasParent()); // speeeeed. Just check before calling

	mat4 m(1.0f);
	parent_cachePtr->localTransformRecursive(&m);
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

	if (parent_cachePtr != nullptr)
		parent_cachePtr->globalTransformRecursive(m);
}

glm::mat4 Entity::GetGlobalToLocalMatrix() const {
	assert(HasParent()); // speeeeed. Just check before calling

	mat4 m(1.0f);
	parent_cachePtr->globalTransformRecursive(&m);
	return m;
}

nlohmann::json Entity::serializeJson() const {

	nlohmann::json j;
	j["id"] = ID;
	j["name"] = name;
	if (HasParent()) {
		j["parent"] = parent;
	}
	if (children.size() > 0) {
		for (auto& c : children) {
			j["children"].push_back((entityID)c);
		}
	}
	j["transform"] = transform.serializeJson();

	return j;
}
void Entity::deserializeJson(const nlohmann::json& j, Entity* e) {

	e->ID = j["id"].get<int>();
	e->name = j["name"].get<std::string>();
	e->transform = Transform::deserializeJson(j["transform"]);
	if (j.contains("parent")) {
		e->parent = j["parent"];
	}
	if (j.contains("children")) {
		for (auto& c : j["children"]) {
			e->children.insert(static_cast<entityID>(c));
		}
	}
}

Entity::Entity(const AssetPack::Entity* packEntity) {

	ID = packEntity->id();
	name = packEntity->name()->str();
	transform = Transform(packEntity->transform());
}