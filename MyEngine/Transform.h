#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "serialization.h"

#include <assetPack/common_generated.h>



struct Transform {
	glm::vec2 position = glm::vec2(0.0f);
	glm::vec2 scale = glm::vec2(1.0f);
	float rotation = 0.0f;

	Transform(glm::vec2 position = glm::vec2(0.0f), glm::vec2 scale = glm::vec2(1.0f), float rotation = 0.0f) {
		this->position = position;
		this->scale = scale;
		this->rotation = rotation;
	}

	nlohmann::json serializeJson() const {
		nlohmann::json j;
		j["position"] = { position.x, position.y };
		j["scale"] = { scale.x, scale.y };
		j["rotation"] = rotation;
		return j;
	}

	static Transform deserializeJson(const nlohmann::json& j) {
		Transform t;
		t.position = glm::vec2(j["position"][0], j["position"][1]);
		t.scale = glm::vec2(j["scale"][0], j["scale"][1]);
		t.rotation = j["rotation"].get<float>();
		return t;
	}

	Transform(const AssetPack::Transform* t) :
		position(fromAP(t->position())),
		scale(fromAP(t->scale())),
		rotation(t->rotation())
	{}
};