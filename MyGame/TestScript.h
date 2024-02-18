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
	//BEHAVIOUR_CONSTUCTOR(TestScript);
	TestScript(behavioiurHash b, ComponentAccessor* a, Entity* e) : Behaviour(b, a, e) {
	
		int tt = 0;
	};

	PROPERTY_EXPORT(
		MAKE_PROPERTY(speed),
		MAKE_PROPERTY(direction)
	);

	float speed = 0.3f;
	vec2 direction = { 1.0f, 0.0f };

	//std::vector<SerializableProperty> getProperties() override {
	//	return { makeProperty("speed", &speed), makeProperty("direction", &direction) };
	//};

	void Start() override {
	}

	void Update() override {

		transform->position += direction * speed * deltaTime;

	}

};


