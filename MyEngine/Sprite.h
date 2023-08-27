#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

#include "typedefs.h"


const auto Sprite_extension = ".sprite";


struct AtlasEntry {
	std::string name;
	glm::vec2 uv_min;
	glm::vec2 uv_max;

	static AtlasEntry deserializeJson(nlohmann::json j) {
		AtlasEntry entry;
		entry.name = j["name"];
		entry.uv_min.x = j["u_min"];
		entry.uv_min.y = j["v_min"];
		entry.uv_max.x = j["u_max"];
		entry.uv_max.y = j["v_max"];

		return entry;
	}
};

class Sprite {

public:


	spriteID ID;

	texID texture;
	glm::vec2 resolution;

	std::string fileName;

	std::vector<AtlasEntry> Atlas;

	FilterMode filterMode;

	void serializeJson(std::string filepath) {
		nlohmann::json ja;
		nlohmann::json j;

		j["spriteID"] = ID;
		j["textureID"] = texture;
		j["resolution"] = toJson(resolution);
		j["fileName"] = fileName;
		j["filterMode"] = static_cast<uint32_t>(filterMode);

		ja["sprite"] = j;

		std::ofstream output(filepath);
		output << ja.dump(4) << std::endl;
		output.close();
	}

	static std::shared_ptr<Sprite> deserializeJson(std::string filepath) {
		std::ifstream input(filepath);
		nlohmann::json ja;
		input >> ja;

		nlohmann::json j = ja["sprite"];

		auto sprite = std::make_shared<Sprite>();

		sprite->ID = j["spriteID"];
		sprite->texture = j["textureID"];

		sprite->resolution = fromJson<glm::vec2>(j["resolution"]);
		sprite->fileName = j["fileName"];
		sprite->filterMode = static_cast<FilterMode>(j["filterMode"]);

		if (j.contains("AtlasEntries")) {
			for (auto& i : j["AtlasEntries"]) {
				sprite->Atlas.push_back(AtlasEntry::deserializeJson(i));
			}
		}
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
					sprite->Atlas.push_back(entry);
				}
			}
		}


		return sprite;
	}
};

