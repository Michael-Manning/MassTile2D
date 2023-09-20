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

	std::vector<charQuad>* CalculateQuads(std::shared_ptr<Font> f) {

		if (dirty) {
			quads.clear();
			quads.reserve(text.length());

			glm::vec2 cursor = glm::vec2(0.0f);
			for (int i = 0; i < text.length(); i++) {
				char c = text[i];

				if (c == '\n') {
					cursor.x = 0.0f;
					cursor.y -= f->lineGap;
					continue;
				}

				auto packed = f->operator[](c);
				charQuad q;
				q.uvmax = packed.uvmax;
				q.uvmin = packed.uvmin;
				q.scale = packed.scale;
				q.position = glm::vec2(cursor.x + packed.xOff, cursor.y + packed.yOff - f->baseline);
				cursor.x += packed.advance;
				cursor.x += f->kerningTable[f->kernHash(c, text[i + 1])];
				quads.push_back(q);
			}
			dirty = false;
		}

		return &quads;
	};

private:

	std::vector<charQuad> quads;
};
