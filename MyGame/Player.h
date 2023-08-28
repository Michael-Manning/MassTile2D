#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <typeinfo>
#include <memory>

#include "typedefs.h"
#include "ECS.h"

using namespace glm;

class Player : public Entity {


public:
	std::string GetEditorName() override { // deprecate
		return "Player";
	};

	uint32_t getBehaviorHash() override {
		return cHash("Player");
	};


	// settings
	const float groundAccel = 1.0f;
	const float airAccel = 0.5f;
	const float topMoveSpeed = 5.0f; // max speed achieved through self movement
	const float terminalVelocity = 8.0f; // maximum speed achieved through gravity acceleration

	// variables
	vec2 velocity= vec2(0.0f);
	float grounded = true;

	ColorRenderer* renderer;
	Rigidbody* rigidbody;

	void Start() override {

		renderer = getComponent<ColorRenderer>();
	};
	void Update() override {

		vec2 vel = rigidbody->GetLinearVelocity();

		//left and right movement
		bool left = input->getKey(KeyCode::LeftArrow);
		bool right = input->getKey(KeyCode::RightArrow);

		if (left && !right) {
			velocity += vec2(grounded ? -groundAccel : -airAccel, 0.0f) * DeltaTime;
		}
		else if (!left && right) {
			velocity += vec2(grounded ? groundAccel : airAccel, 0.0f) * DeltaTime;
		}
		


		//if (input->getKeyDown(KeyCode::UpArrow)) {
		//	if (onGround)
		//		rigidbody->AddForce(vec2(0, jumpForce));

		//};
	};
};
