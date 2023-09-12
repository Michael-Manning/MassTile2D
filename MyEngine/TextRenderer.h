#pragma once

#include <vector>
#include <stdint.h>

#include <nlohmann/json.hpp>

#include "typedefs.h"
#include "Component.h"
#include "Font.h"

class TextRenderer : public Component {
public:

	TextRenderer() {};

	TextRenderer(Font font) {
		this->font = font;
	};

	nlohmann::json serializeJson(entityID entId) override;

	static TextRenderer deserializeJson(nlohmann::json j);

	std::string text;

	fontID font;

	TextRenderer duplicate() const {
		assert(false); // damn
		TextRenderer r(font);
		return r;
	};
};
