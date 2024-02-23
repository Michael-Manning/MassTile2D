#include "stdafx.h"

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

#include <assetPack/Sprite_generated.h>

#include "typedefs.h"
#include "serialization.h"

#include "Sprite.h"


Sprite::Sprite(const AssetPack::Sprite* s) {

	ID = s->ID();
	name = s->name()->str();
	resolution = fromAP(s->resolution());
	imageFileName = s->imageFileName()->str();
	filterMode = static_cast<FilterMode>(s->filterMode());

	if (s->atlas() != nullptr) {
		atlas.resize(s->atlas()->size());
		for (size_t i = 0; i < s->atlas()->size(); i++)
			atlas[i] = AtlasEntry::deserializeFlatbuffer(s->atlas()->Get(i));
	}

	if (atlas.size() > 0)
		return;

	auto gridLayout = s->atlasLayout();
	if (gridLayout != nullptr) {
		int xcount = gridLayout->xCount();
		int ycount = gridLayout->yCount();

		float uvw = 1.0f / xcount;
		float uvh = 1.0f / ycount;
		for (size_t i = 0; i < ycount; i++) {
			for (size_t j = 0; j < xcount; j++) {
				AtlasEntry entry;
				entry.name = std::to_string((int)(i + j * xcount));
				entry.uv_min = glm::vec2(j * uvw, i * uvh);
				entry.uv_max = entry.uv_min + glm::vec2(uvw, uvh);
				atlas.push_back(entry);
			}
		}
	}
	return;
}