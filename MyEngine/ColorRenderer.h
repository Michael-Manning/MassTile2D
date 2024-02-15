#pragma once

#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>

#include "typedefs.h"
#include "Component.h"
#include "serialization.h"

#include <assetPack/common_generated.h>

class ColorRenderer : public Component {
public:

	enum class Shape {
		Rectangle,
		Circle
	};

	ColorRenderer() {};
	ColorRenderer(glm::vec4 color, Shape shape = Shape::Rectangle) {
		this->color = color;
		this->shape = shape;
	}

	nlohmann::json serializeJson(entityID entId) const override;

	static ColorRenderer deserializeJson(nlohmann::json);

	glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	Shape shape;

	ColorRenderer duplicate() const{
		ColorRenderer r(color, shape);
		return r;
	};

	static ColorRenderer deserializeFlatbuffers(const AssetPack::ColorRenderer* c) {
		ColorRenderer r;
		r.color = fromAP(c->color());
		r.shape = static_cast<Shape>(c->shape());
		return r;
	};
};
