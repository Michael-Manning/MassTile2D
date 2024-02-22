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

	nlohmann::json serializeJson() const;
	static Transform deserializeJson(const nlohmann::json& j);

	static Transform deserializeFlatbuffers(const AssetPack::Transform* t) {
		return Transform(
			fromAP(t->position()),
			fromAP(t->scale()),
			t->rotation());
	}
};