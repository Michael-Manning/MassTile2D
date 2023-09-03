#pragma once

#include <string>

#include <nlohmann/json.hpp>

struct NoiseParams {
	std::string nodeTree;
	float min;
	float frequency;

	static NoiseParams fromJson(const nlohmann::json& j) {
		NoiseParams p;
		p.nodeTree = j["nodeTree"];
		p.min = j["min"];
		p.min = p.min / 2.0f + 0.5f;
		p.frequency = j["frequency"];
		return p;
	}
};
