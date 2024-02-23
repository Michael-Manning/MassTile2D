#pragma once

#include <optional>
#include <string>
#include <fstream>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "ECS.h"
#include "serialization.h"
#include "SceneData.h"

#include <nlohmann/json.hpp>
#include <robin_hood.h>
#include <assetPack/SceneData_generated.h>
#include <assetPack/Prefab_generated.h>

const auto Prefab_extension = ".prefab";

class Prefab {
public:

	Prefab() {}

	std::string name;

	SceneData sceneData;

	entityID TopLevelEntity = NULL_Entity; // prefabs can only have one enity with no parent

	static std::string peakJsonName(std::string filename) {
		checkAppend(filename, Prefab_extension);
		std::ifstream input(filename);
		nlohmann::json j;
		input >> j;
		return static_cast<std::string>(j["name"]);
	}

	void serializeJson(std::string filename) const;
	static void deserializeJson(std::string filename, Prefab* prefab);
	Prefab(const AssetPack::Prefab* s);

private:
	void FindTopLevelEntity();
};