#include "stdafx.h"

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <set>
#include <optional>
#include <memory>
#include <typeinfo>
#include <robin_hood.h>

#include "typedefs.h"
#include "ECS.h"
#include "Component.h"
#include "Utils.h"
#include "Input.h"
#include "serialization.h"

#include "Scene.h"

#include <assetPack/common_generated.h>

#include "Behaviour.h"


nlohmann::json SerializableProperty::serializeJson() const {

	nlohmann::json j;
	j["type"] = type;
	j["name"] = name;

	if (type == SerializableProperty::Type::INT)
		j["value"] = *reinterpret_cast<int*>(value);
	else if (type == SerializableProperty::Type::FLOAT)
		j["value"] = *reinterpret_cast<float*>(value);
	else if (type == SerializableProperty::Type::VEC2)
		j["value"] = toJson(*reinterpret_cast<glm::vec2*>(value));
	else {
		assert(false);
	}

	return j;
}


void SerializableProperty::assignValue(const nlohmann::json& j) {
	if (type == SerializableProperty::Type::INT)
		*reinterpret_cast<int*>(value) = static_cast<int>(j["value"]);
	else if (type == SerializableProperty::Type::FLOAT)
		*reinterpret_cast<float*>(value) = static_cast<float>(j["value"]);
	else if (type == SerializableProperty::Type::VEC2)
		*reinterpret_cast<glm::vec2*>(value) = fromJson<glm::vec2>(j["value"]);
	else {
		assert(false);
	}
}

//void SerializableProperty::deserialize(const nlohmann::json& j, SerializableProperty* sp) {
//	sp->type = j["type"];
//	sp->name = j["name"];
//
//	if (sp->type == SerializableProperty::Type::INT)
//		*reinterpret_cast<int*>(sp->value) = static_cast<int>(j["value"]);
//	else if (sp->type == SerializableProperty::Type::FLOAT)
//		*reinterpret_cast<float*>(sp->value) = static_cast<float>(j["value"]);
//	else if (sp->type == SerializableProperty::Type::VEC2)
//		*reinterpret_cast<glm::vec2*>(sp->value) = static_cast<glm::vec2>(j["value"]);
//	else {
//		assert(false);
//	}
//}
void SerializableProperty::deserialize(const AssetPack::SerializableProperty* s, SerializableProperty* sp) {
	sp->type = static_cast<SerializableProperty::Type>(s->type());
	sp->name = s->name()->str();

	if (sp->type == SerializableProperty::Type::INT)
		*reinterpret_cast<int*>(sp->value) = s->value_as_U_int()->value();
	else if (sp->type == SerializableProperty::Type::FLOAT)
		*reinterpret_cast<float*>(sp->value) = s->value_as_U_float()->value();
	else if (sp->type == SerializableProperty::Type::VEC2)
		*reinterpret_cast<glm::vec2*>(sp->value) = fromAP(s->value_as_U_vec2()->value());
	else {
		assert(false);
	}
}


template <>
ColorRenderer* Behaviour::getComponent() {
	return &accessor->colorRenderers->at(entity->ID);
}

template <>
SpriteRenderer* Behaviour::getComponent() {
	return &accessor->spriteRenderers->at(entity->ID);
}

template <>
TextRenderer* Behaviour::getComponent() {
	return &accessor->textRenderers->at(entity->ID);
}

template <>
ParticleSystemRenderer* Behaviour::getComponent() {
	return &accessor->particleSystemRenderers->at(entity->ID);
}

template <>
Staticbody* Behaviour::getComponent() {
	return &accessor->staticbodies->at(entity->ID);
}

template <>
Rigidbody* Behaviour::getComponent() {
	return &accessor->rigidbodies->at(entity->ID);
}

Input* Behaviour::input = nullptr;
float Behaviour::deltaTime = 0.0f;

void Behaviour::Destory() {
	accessor->scene->DeleteEntity(entity->ID, true);
}