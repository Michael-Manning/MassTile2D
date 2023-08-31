#include <vector>
#include <stdint.h>
#include <memory>
#include <cassert>
#include <unordered_map>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"


#include "engine.h"
#include "pipelines.h"
#include "Physics.h"
#include "serialization.h"

#include "ECS.h"
#include "BehaviorRegistry.h"

using namespace glm;
using namespace std;

using namespace nlohmann;

json Transform::serializeJson() {
	json j;
	j["position"] = { position.x, position.y };
	j["scale"] = { scale.x, scale.y };
	j["rotation"] = rotation;
	return j;
}
Transform Transform::deserializeJson(const nlohmann::json& j) {
	Transform t;
	t.position = glm::vec2(j["position"][0], j["position"][1]);
	t.scale = glm::vec2(j["scale"][0], j["scale"][1]);
	t.rotation = j["rotation"].get<float>();
	return t;
}

json Entity::serializeJson() {

	nlohmann::json j;
	j["id"] = ID;
	j["name"] = name;
	if (parent.has_value()) {
		j["parent"] = parent.value();
	}
	if (children.size() > 0) {
		for (auto& c : children) {
			j["parent"].push_back((entityID)c);
		}
	}
	j["transform"] = transform.serializeJson();

	j["behaviorHash"] = getBehaviorHash();

	return j;
}
std::shared_ptr<Entity> Entity::deserializeJson(const nlohmann::json& j) {
	std::shared_ptr<Entity> e;
	uint32_t hash = j["behaviorHash"];
	if (hash != 0)
		e = BehaviorMap[hash].second();
	else
		e = make_shared<Entity>();

	e->ID = j["id"].get<int>();
	e->name = j["name"].get<string>();
	e->transform = Transform::deserializeJson(j["transform"]);
	if (j.contains("parent")) {
		e->parent = j["parent"];
	}
	if (j.contains("children")) {
		for (auto& c : j["children"]) {
			e->children.insert(static_cast<entityID>(c));
		}
	}
	return e;
}


nlohmann::json SpriteRenderer::serializeJson(entityID entId) {
	nlohmann::json j;
	j["entityID"] = entId;
	j["spriteID"] = sprite;
	j["atlasIndex"] = atlasIndex;
	return j;
}
SpriteRenderer SpriteRenderer::deserializeJson(nlohmann::json j) {
	spriteID id = j["spriteID"].get<uint32_t>();
	SpriteRenderer s(id);
	s.atlasIndex = j["atlasIndex"];
	return s;
}


nlohmann::json ColorRenderer::serializeJson(entityID entId) {
	nlohmann::json j;
	j["entityID"] = entId;
	j["color"] = toJson(color); //{color.r, color.g, color.b, color.a};
	j["shape"] = (int)shape;

	return j;
}
ColorRenderer ColorRenderer::deserializeJson(nlohmann::json j) {

	ColorRenderer s;
	s.color = fromJson<vec4>(j["color"]);
	s.shape = j["shape"];
	
	return s;
}


nlohmann::json Rigidbody::serializeJson(entityID entId) {
	nlohmann::json j;
	j["entityID"] = entId;

	j["linearDamping"] = GetLinearDamping();
	j["angularDamping"] = GetAngularDamping();
	j["fixedRotation"] = GetFixedRotation();
	j["bullet"] = GetBullet();
	j["gravityScale"] = GetGravityScale();
	j["friction"] = GetFriction();
	j["density"] = GetDensity();
	j["restitution"] = GetRestitution();

	j["collider"] = collider->serializeJson();

	return j;
}
Rigidbody Rigidbody::deserializeJson(const nlohmann::json& j, std::shared_ptr<b2World> world) {
	auto collider = Collider_deserializeJson(j["collider"]);
	Rigidbody r(collider);

	b2BodyDef bdef;
	bdef.linearDamping = j["linearDamping"];
	bdef.angularDamping = j["angularDamping"];
	bdef.fixedRotation = j["fixedRotation"];
	bdef.bullet = j["bullet"];
	bdef.gravityScale = j["gravityScale"];

	b2FixtureDef fdef;
	fdef.friction = j["friction"];
	fdef.density = j["density"];
	fdef.restitution = j["restitution"];

	r._generateBody(world, &fdef, &bdef);

	return r;
}

nlohmann::json Staticbody::serializeJson(entityID entId) {
	nlohmann::json j;
	j["entityID"] = entId;

	j["collider"] = collider->serializeJson();

	return j;
}
Staticbody Staticbody::deserializeJson(const nlohmann::json& j, std::shared_ptr<b2World> world) {
	auto collider = Collider_deserializeJson(j["collider"]);
	Staticbody s(world, collider);
	return s;
}


nlohmann::json TileWorldRenderer::serializeJson(entityID entId) {
	nlohmann::json j;
	j["entityID"] = entId;
	j["spriteID"] = sprite;
	return j;
}
TileWorldRenderer TileWorldRenderer::deserializeJson(nlohmann::json j) {
	spriteID id = j["spriteID"].get<uint32_t>();
	TileWorldRenderer s(id);
	return s;
}
