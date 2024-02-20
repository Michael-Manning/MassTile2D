#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <typeinfo>
#include <memory>
#include <iostream>

#include "typedefs.h"
#include "ECS.h"
#include "global.h"
#include "worldGen.h"
#include "TileWorld.h"
#include "Player.h"
#include "Behaviour.h"

using namespace glm;
using namespace std;


class TestScript : public Behaviour {
public:
	BEHAVIOUR_CONSTUCTOR(TestScript){}
	BEHAVIOUR_CLONE(TestScript);


	PROPERTY_EXPORT(
		MAKE_PROPERTY(speed),
		MAKE_PROPERTY(direction)
	);

	float speed = 0.3f;
	vec2 direction = { 1.0f, 0.0f };


	void Start() override {
	}

	void Update() override {

		transform->position += direction * speed * deltaTime;

	}

};


