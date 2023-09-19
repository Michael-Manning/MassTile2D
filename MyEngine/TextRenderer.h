#pragma once

#include <vector>
#include <stdint.h>
#include <memory>

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include "typedefs.h"
#include "Component.h"
#include "Font.h"

class TextRenderer : public Component {
public:

	TextRenderer() {};

	TextRenderer(fontID font) {
		this->font = font;
	};

	nlohmann::json serializeJson(entityID entId) override;

	static TextRenderer deserializeJson(nlohmann::json j);

	std::string text;
	glm::vec4 color;

	fontID font;

	bool dirty = true;

	TextRenderer duplicate() const {
		assert(false); // damn
		TextRenderer r(font);
		return r;
	};

	// user dirty flag and cache
	std::vector<charQuad> CalculateQuads(std::shared_ptr<Font> f) {

		std::vector<charQuad> quads;
		quads.reserve(text.length());

		float cursor = 0;
		//for (char c : text) {
		for (int i = 0; i < text.length(); i++){
			char c = text[i];
			auto packed = f->operator[](c);
			charQuad q;
			q.uvmax = packed.uvmax;
			q.uvmin = packed.uvmin;
			q.scale = packed.scale;
			q.position = glm::vec2(cursor + packed.xOff, packed.yOff);
			cursor += packed.advance;
			cursor += f->kerningTable[f->kernHash(c, text[i + 1])];
			quads.push_back(q);
		}

		return quads;
	};

private:

	//Font* cachedFont
};
