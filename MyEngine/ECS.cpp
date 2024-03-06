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

nlohmann::json TextRenderer::serializeJson(entityID entId) const {
	nlohmann::json j;
	j["entityID"] = entId;
	j["fontID"] = font;
	j["color"] = toJson(color);
	j["text"] = text;
	return j;
}
TextRenderer TextRenderer::deserializeJson(nlohmann::json j){
	fontID id = j["fontID"].get<uint32_t>();
	TextRenderer t(id);
	t.text = j["text"];
	t.color = fromJson <glm::vec4>(j["color"]);
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

	j["linearDamping"] = desc.linearDamping;
	j["angularDamping"] = desc.angularDamping;
	j["fixedRotation"] = desc.fixedRotation;
	j["bullet"] = desc.bullet;
	j["gravityScale"] = desc.gravityScale;
	j["friction"] = desc.friction;
	j["density"] = desc.density;
	j["restitution"] = desc.restitution;

	//j["linearDamping"] = GetLinearDamping();
	//j["angularDamping"] = GetAngularDamping();
	//j["fixedRotation"] = GetFixedRotation();
	//j["bullet"] = GetBullet();
	//j["gravityScale"] = GetGravityScale();
	//j["friction"] = GetFriction();
	//j["density"] = GetDensity();
	//j["restitution"] = GetRestitution();

	j["collider"] = collider.serializeJson();

	return j;
}

nlohmann::json Staticbody::serializeJson(entityID entId) const {
	nlohmann::json j;
	j["entityID"] = entId;

	j["collider"] = collider.serializeJson();

	return j;
}

