#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <typeinfo>
#include <memory>

#include "typedefs.h"
#include "ECS.h"

using namespace glm;

// offset toward zero with floor
glm::vec2 decellerate(const glm::vec2& A, const glm::vec2& B) {
	glm::vec2 result;

	// Handle the x-component
	if (A.x < 0) {
		result.x = A.x + B.x;
	}
	else {
		result.x = A.x - B.x;
	}
	if ((A.x > 0 && result.x < 0) || (A.x < 0 && result.x > 0)) {
		result.x = 0;
	}

	// Handle the y-component
	if (A.y < 0) {
		result.y = A.y + B.y;
	}
	else {
		result.y = A.y - B.y;
	}
	if ((A.y > 0 && result.y < 0) || (A.y < 0 && result.y > 0)) {
		result.y = 0;
	}

	return result;
}

class Player : public Entity {


public:
	std::string GetEditorName() override { // deprecate
		return "Player";
	};

	uint32_t getBehaviorHash() override {
		return cHash("Player");
	};


	// settings
	const float groundAccel = 50.0f;
	const float airAccel = 30.0f;
	const float topMoveSpeed = 4.0f; // max speed achieved through self movement
	const float terminalVelocity = 8.0f; // maximum speed achieved through gravity acceleration
	const float idleDecelleratiom = 45.0f;

	// variables
	vec2 velocity= vec2(0.0f);
	float grounded = true;

	ColorRenderer* renderer;
	Rigidbody* rigidbody;

	void Start() override {

		renderer = getComponent<ColorRenderer>();
	};
	void Update() override {

		//vec2 vel = rigidbody->GetLinearVelocity();

		//left and right movement
		bool left = input->getKey(KeyCode::LeftArrow);
		bool right = input->getKey(KeyCode::RightArrow);

		if (left && !right) {
			velocity += vec2(grounded ? -groundAccel : -airAccel, 0.0f) * DeltaTime;
		}
		else if (!left && right) {
			velocity += vec2(grounded ? groundAccel : airAccel, 0.0f) * DeltaTime;
		}
		else if (!left && !right) {
			if (grounded) {
				if (velocity.x)
					velocity = decellerate(velocity, vec2(idleDecelleratiom * DeltaTime, 0.0f));
			}
		}
		
		//velocity.x = glm::clamp(velocity.x, -topMoveSpeed, topMoveSpeed);
		//velocity.x = glm::clamp(velocity.y, -terminalVelocity, terminalVelocity);

		transform.position += velocity * DeltaTime;



		//if (input->getKeyDown(KeyCode::UpArrow)) {
		//	if (onGround)
		//		rigidbody->AddForce(vec2(0, jumpForce));

		//};
	};
};
