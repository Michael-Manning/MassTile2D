#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <set>
#include <optional>
#include <typeinfo>

#include "typedefs.h"
#include "ECS.h"

using namespace glm;

class Ball : public Entity {

public:
	
	std::string GetEditorName() override { // deprecate
		return "Ball"; 
	}

	uint32_t getBehaviorHash() override {
		return cHash("Ball");
	}

private:

	ColorRenderer* cr = nullptr;

	virtual void Start() {
		cr = getComponent<ColorRenderer>();
	};
	virtual void Update() {
		cr->color = vec4(0.0, 1.0, 1.0, 1.0);
	};
};
