#include "stdafx.h"

#include <vector>
#include <stdint.h>
#include <memory>
#include <cassert>
#include <unordered_map>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"


#include "engine.h"
#include "Physics.h"
#include "serialization.h"

#include "ParticleSystemPL.h"
#include "ECS.h"
#include "BehaviorRegistry.h"

using namespace glm;
using namespace std;

using namespace nlohmann;

json Transform::serializeJson() const {
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

json Entity::serializeJson() const {

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

	j["behaviorHash"] = getBehaviorHash();

	return j;
}
void Entity::deserializeJson(const nlohmann::json& j, Entity* e) {

	uint32_t hash = j["behaviorHash"];
	if (hash != 0)
		assert(false);
	//	e = BehaviorMap[hash].second();
	//else
	//	e = make_shared<Entity>();

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
}

void Entity::deserializeFlatbuffers(const AssetPack::Entity* packEntity, Entity* entity) {

	uint32_t hash = packEntity->behaviorHash();
	if (hash != 0)
		assert(false);
	//	entity = BehaviorMap[hash].second();
	//else
	//	entity = make_shared<Entity>();

	entity->ID = packEntity->id();
	entity->name = packEntity->name()->str();
	entity->transform = Transform::deserializeFlatbuffers(packEntity->transform());
}


nlohmann::json ColorRenderer::serializeJson(entityID entId) const {
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

nlohmann::json SpriteRenderer::serializeJson(entityID entId) const {
	nlohmann::json j;
	j["entityID"] = entId;
	j["spriteID"] = sprite;
	j["atlasIndex"] = atlasIndex;
	return j;
}
SpriteRenderer SpriteRenderer::deserializeJson(nlohmann::json j){
	spriteID id = j["spriteID"].get<uint32_t>();
	SpriteRenderer s(id);
	s.atlasIndex = j["atlasIndex"];
	return s;
}

nlohmann::json TextRenderer::serializeJson(entityID entId) const {
	nlohmann::json j;
	j["entityID"] = entId;
	j["fontID"] = font;
	j["text"] = text;
	return j;
}
TextRenderer TextRenderer::deserializeJson(nlohmann::json j){
	fontID id = j["fontID"].get<uint32_t>();
	TextRenderer t(id);
	t.text = j["text"];
	return t;
}


nlohmann::json ParticleSystemPL::ParticleSystemConfiguration::serializeJson() const {
	nlohmann::json j;

	j["particleCount"] = particleCount;
	j["burstMode"] = burstMode;
	j["spawnRate"] = spawnRate;
	j["particleLifeSpan"] = particleLifeSpan;
	j["gravity"] = gravity;
	j["startSize"] = startSize;
	j["endSize"] = endSize;
	j["startColor"] = toJson(startColor);
	j["endColor"] = toJson(endColor);

	return j;
};
void ParticleSystemPL::ParticleSystemConfiguration::deserializeJson(nlohmann::json j, ParticleSystemPL::ParticleSystemConfiguration* config) {

	config->particleCount = j["particleCount"];
	config->burstMode = j["burstMode"];
	config->spawnRate = j["spawnRate"];
	config->particleLifeSpan = j["particleLifeSpan"];
	config->gravity = j["gravity"];
	config->startSize = j["startSize"];
	config->endSize = j["endSize"];
	config->startColor = fromJson<vec4>(j["startColor"]);
	config->endColor = fromJson<vec4>(j["endColor"]);
};
void ParticleSystemPL::ParticleSystemConfiguration::deserializeFlatbuffers(const AssetPack::ParticleSystemConfiguration* p, ParticleSystemPL::ParticleSystemConfiguration* config) {

	config->particleCount = p->particleCount();
	config->burstMode = p->burstMode();
	config->spawnRate = p->spawnRate();
	config->particleLifeSpan = p->particleLifeSpan();
	config->gravity = p->gravity();
	config->startSize = p->startSize();
	config->endSize = p->endSize();
	config->startColor = fromAP(p->startColor());
	config->endColor = fromAP(p->endColor());
};


nlohmann::json Rigidbody::serializeJson(entityID entId) const {
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
Rigidbody Rigidbody::deserializeJson(const nlohmann::json& j) {
	auto collider = Collider_deserializeJson(j["collider"]);
	Rigidbody r(collider);

	r.desc.linearDamping = j["linearDamping"];
	r.desc.angularDamping = j["angularDamping"];
	r.desc.fixedRotation = j["fixedRotation"];
	r.desc.bullet = j["bullet"];
	r.desc.gravityScale = j["gravityScale"];

	r.desc.friction = j["friction"];
	r.desc.density = j["density"];
	r.desc.restitution = j["restitution"];


	return r;
}

nlohmann::json Staticbody::serializeJson(entityID entId) const {
	nlohmann::json j;
	j["entityID"] = entId;

	j["collider"] = collider->serializeJson();

	return j;
}
Staticbody Staticbody::deserializeJson(const nlohmann::json& j) {
	auto collider = Collider_deserializeJson(j["collider"]);
	Staticbody s(collider);
	return s;
}
