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

using namespace glm;
using namespace std;

// offset toward zero with floor
glm::vec2 decellerate(const glm::vec2& A, const glm::vec2& B) {
	glm::vec2 result;

	if (A.x < 0) {
		result.x = A.x + B.x;
	}
	else {
		result.x = A.x - B.x;
	}
	if ((A.x > 0 && result.x < 0) || (A.x < 0 && result.x > 0)) {
		result.x = 0;
	}

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
	const float groundAccel = 40.0f;
	const float airAccel = 30.0f;
	const float topMoveSpeed = 8.0f; // max speed achieved through self movement
	const float terminalVelocity = 18.0f; // maximum speed achieved through gravity acceleration
	const float idleDecelleratiom = 45.0f;
	const float gravityAccel = 18;
	const float jumpVel = 8;

	// variables
	vec2 velocity = vec2(0.0f);
	float grounded = true;

	ColorRenderer* renderer;
	Rigidbody* rigidbody;


	bool queryTile(vec2 pos) {
		int tileX = pos.x / tileWorldSize + mapW / 2;
		int tileY = pos.y / tileWorldSize + mapH / 2;

		if (tileX > 0 && tileX < mapW && tileY > 0 && tileY < mapH) {

			tileY = mapH - tileY - 1;

			blockID block = tileWolrdGlobalRef->getTile(tileX, tileY);

			return block != Tiles::Air;
		}
	}

	ivec2 getTileXY(vec2 pos) {
		int tileX = pos.x / tileWorldSize + mapW / 2;
		int tileY = pos.y / tileWorldSize + mapH / 2;

		if (tileX > 0 && tileX < mapW && tileY > 0 && tileY < mapH)
			return ivec2(tileX, mapH - tileY - 1);
		return ivec2(-1, -1);
	}

	void Start() override {

	};
	void Update() override {

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

		if (input->getKeyDown(KeyCode::UpArrow) && grounded) {
			velocity.y = jumpVel;
			grounded = false;
		}
		else {
			velocity.y -= gravityAccel * DeltaTime;
		}

		velocity.x = glm::clamp(velocity.x, -topMoveSpeed, topMoveSpeed);
		velocity.y = glm::clamp(velocity.y, -terminalVelocity, terminalVelocity);


		vec2 npos = transform.position + velocity * DeltaTime;

		//int dx = velocity.x == 0 ? 0 : velocity.x < 0 ? -1 : 1;
		//int dy = velocity.y == 0 ? 0 : velocity.y < 0 ? -1 : 1;

		vector<vec2> tpoints(4);
		float sx = transform.scale.x / 2.0;
		float sy = transform.scale.y / 2.0;
		tpoints[0] = vec2(sx, sy);
		tpoints[1] = vec2(-sx, sy);
		tpoints[2] = vec2(sx, -sy);
		tpoints[3] = vec2(-sx, -sy);

		float resolvedXAvg = 0.0f;
		int avgSamplesX = 0;

		float resolvedYAvg = 0.0f;
		int avgSamplesY = 0;


		for (size_t i = 0; i < 4; i++) {

			if (queryTile(npos + tpoints[i]) == false)
				continue;

			ivec2 prevTile = getTileXY(transform.position + tpoints[i]);
			ivec2 curTile = getTileXY(npos + tpoints[i]);

			int dx = curTile.x - prevTile.x;
			int dy = -(curTile.y - prevTile.y);

			if (dx != 0) {
				if (dx > 0)
					resolvedXAvg += (curTile.x - mapW / 2) * tileWorldSize -sx - 0.0001f;
				else
					resolvedXAvg += (curTile.x - mapW / 2 + 1) * tileWorldSize + sx + 0.0001f;
				avgSamplesX++;
				velocity.x = 0;
			}
			if (dy != 0) {
				if (dy > 0) {
					resolvedYAvg += (mapH / 2 - curTile.y - 1) * tileWorldSize - sy - 0.0001f;
				}
				else {
					resolvedYAvg += (mapH / 2 - curTile.y) * tileWorldSize + sy + 0.0001f;
					grounded = true;
				}
				avgSamplesY++;
				velocity.y = 0;
			}
		}

		if (avgSamplesX > 0) 
			npos.x = resolvedXAvg / (float)avgSamplesX;
		if (avgSamplesY > 0)
			npos.y = resolvedYAvg / (float)avgSamplesY;

		transform.position = npos;

		if (transform.position != GcameraPos) {
			vec2 npos = GcameraPos + (transform.position - GcameraPos) * 4.0f * DeltaTime;
			if (glm::distance(npos, transform.position) > glm::distance(GcameraPos, transform.position)) {
				GcameraPos = transform.position;
			}
			else {
				GcameraPos = npos;
			}
		}
	};
};
