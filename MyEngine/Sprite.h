#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

#include <assetPack/Sprite_generated.h>

#include "typedefs.h"
#include "serialization.h"


const auto Sprite_extension = ".sprite";


struct AtlasEntry {
	std::string name;
	glm::vec2 uv_min;
	glm::vec2 uv_max;

	static AtlasEntry deserializeJson(nlohmann::json j) {
		AtlasEntry entry;
		entry.name = j["name"];
		entry.uv_min = fromJson<glm::vec2>(j["uv_min"]);
		entry.uv_max = fromJson<glm::vec2>(j["uv_max"]);

		return entry;
	}
	static AtlasEntry deserializeFlatbuffer(const AssetPack::AtlasEntry* e) {
		AtlasEntry entry;
		entry.name = e->name()->str();
		entry.uv_min = fromAP(e->uv_min());
		entry.uv_max = fromAP(e->uv_max());

		return entry;
	};
};

class Sprite {

public:

	std::string name;

	spriteID ID;

	texID textureID;
	glm::vec2 resolution;

	std::string imageFileName;

	std::vector<AtlasEntry> atlas;

	FilterMode filterMode;

	Sprite() {};

	Sprite(const AssetPack::Sprite* s);

	void serializeJson(std::string filepath) const {
		nlohmann::json j;

		j["ID"] = ID;
		j["name"] = name;
		//j["textureID"] = textureID;
		j["resolution"] = toJson(resolution);
		j["imageFileName"] = imageFileName;
		j["filterMode"] = static_cast<uint32_t>(filterMode);


		std::ofstream output(filepath);
		output << j.dump(4) << std::endl;
		output.close();
	}

	static void deserializeJson(std::string filepath, Sprite* sprite) {

		std::ifstream input(filepath);

		nlohmann::json j;
		input >> j;

		sprite->ID = j["ID"];
		sprite->name = j["name"];

		sprite->resolution = fromJson<glm::vec2>(j["resolution"]);
		sprite->imageFileName = j["imageFileName"];
		sprite->filterMode = static_cast<FilterMode>(j["filterMode"]);

		if (j.contains("atlas")) {
			for (auto& i : j["atlas"]) {
				sprite->atlas.push_back(AtlasEntry::deserializeJson(i));
			}
		}

		// gen atlas via grid definition
		else if (j.contains("atlasLayout")) {
			nlohmann::json jb = j["atlasLayout"];
			int xcount = jb["xCount"];
			int ycount = jb["yCount"];

			float uvw = 1.0f / xcount;
			float uvh = 1.0f / ycount;
			for (size_t i = 0; i < ycount; i++) {
				for (size_t j = 0; j < xcount; j++) {
					AtlasEntry entry;
					entry.name = std::to_string((int)(i + j * xcount));
					entry.uv_min = glm::vec2(j * uvw, i * uvh);
					entry.uv_max = entry.uv_min + glm::vec2(uvw, uvh);
					sprite->atlas.push_back(entry);
				}
			}
		}

		return;
	}
};

