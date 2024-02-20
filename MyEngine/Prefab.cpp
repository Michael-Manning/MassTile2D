#include "stdafx.h"

#include <optional>
#include <string>
#include <fstream>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "ECS.h"
#include "serialization.h"

#include <nlohmann/json.hpp>
#include <robin_hood.h>
#include <assetPack/SceneData_generated.h>
#include <assetPack/Prefab_generated.h>

#include "Prefab.h"

using namespace nlohmann;
using namespace std;

void Prefab::serializeJson(std::string filename) const {
	json j;

	j["name"] = name;

	j["sceneData"] = sceneData.serializeJson(true);

	checkAppend(filename, Prefab_extension);
	std::ofstream output(filename);
	output << j.dump(4) << std::endl;
	output.close();
}

void Prefab::deserializeJson(std::string filename, Prefab* prefab) {

	checkAppend(filename, Prefab_extension);
	std::ifstream input(filename);
	json j;
	input >> j;

	prefab->name = j["name"];

	SceneData::deserializeJson(j["sceneData"], &prefab->sceneData);

	prefab->FindTopLevelEntity();
}


void Prefab::deserializeFlatbuffers(const AssetPack::Prefab* s, Prefab* prefab) {
	prefab->name = s->name()->str();
	SceneData::deserializeFlatbuffers(s->sceneData(), &prefab->sceneData);

	prefab->FindTopLevelEntity();
}


void Prefab::FindTopLevelEntity() {
	for (auto& [ID, e] : sceneData.entities)
	{
		if (e.HasParent() == false)
			TopLevelEntity = ID;
	}
}