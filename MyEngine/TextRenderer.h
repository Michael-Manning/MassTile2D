#pragma once

#include <vector>
#include <stdint.h>
#include <memory>

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include "typedefs.h"
#include "Component.h"
#include "Font.h"

#include <assetPack/common_generated.h>

class TextRenderer : public Component {
public:

	TextRenderer() {};

	TextRenderer(fontID font) {
		this->font = font;
	};

	nlohmann::json serializeJson(entityID entId) const override;

	static TextRenderer deserializeJson(nlohmann::json j);

	std::string text;
	glm::vec4 color = glm::vec4(1.0f);

	fontID font;

	bool dirty = true;

	TextRenderer duplicate() const {
		assert(false); // damn
		TextRenderer r(font);
		return r;
	};

	std::vector<charQuad> quads;

	static TextRenderer deserializeFlatbuffers(const AssetPack::TextRenderer* t) {
		TextRenderer r;
		r.color = fromAP(t->color());
		r.font = t->fontID();
		r.text = t->text()->str();
		return r;
	};
};
