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


namespace {
	std::string flatbufferUnionTypeString(SerializableProperty::Type type) {
		if (type == SerializableProperty::Type::INT)
			return "U_int";
		else if (type == SerializableProperty::Type::FLOAT)
			return "U_float";
		else if (type == SerializableProperty::Type::VEC2)
			return "U_vec2";
		else
			assert(false);
		return "";
	}
}

nlohmann::json SerializableProperty::serializeJson() const {

	nlohmann::json j;
	j["type"] = type;
	j["name"] = name;

	// have to format this in an unusual way so support automatic flatbuffer unions
	j["value_type"] = flatbufferUnionTypeString(type);
	nlohmann::json j2;
	{
		if (type == SerializableProperty::Type::INT)
			j2["value"] = *reinterpret_cast<int*>(value);
		else if (type == SerializableProperty::Type::FLOAT)
			j2["value"] = *reinterpret_cast<float*>(value);
		else if (type == SerializableProperty::Type::VEC2)
			j2["value"] = toJson(*reinterpret_cast<glm::vec2*>(value));
		else
			assert(false);
	}
	j["value"] = j2;
	return j;
}


void SerializableProperty::assignValue(const nlohmann::json& j) {
	if (type == SerializableProperty::Type::INT)
		*reinterpret_cast<int*>(value) = static_cast<int>(j["value"]["value"]);
	else if (type == SerializableProperty::Type::FLOAT)
		*reinterpret_cast<float*>(value) = static_cast<float>(j["value"]["value"]);
	else if (type == SerializableProperty::Type::VEC2)
		*reinterpret_cast<glm::vec2*>(value) = fromJson<glm::vec2>(j["value"]["value"]);
	else {
		assert(false);
	}
}

void SerializableProperty::assignValue(const AssetPack::SerializableProperty* s) {

	if (type == SerializableProperty::Type::INT)
		*reinterpret_cast<int*>(value) = s->value_as_U_int()->value();
	else if (type == SerializableProperty::Type::FLOAT)
		*reinterpret_cast<float*>(value) = s->value_as_U_float()->value();
	else if (type == SerializableProperty::Type::VEC2)
		*reinterpret_cast<glm::vec2*>(value) = fromAP(s->value_as_U_vec2()->value());
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