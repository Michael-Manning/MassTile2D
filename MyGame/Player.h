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
#include "Behaviour.h"
#include "Collision.h"

using namespace glm;
using namespace std;

constexpr int PlayerInventorySize = 50;

// offset toward zero with floor
static glm::vec2 decellerate(const glm::vec2& A, const glm::vec2& B) {
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


struct serializableProperty {
	
	enum class serializableType {
		INT,
		FLOAT
	};

	serializableType type;
	std::string name;
	void* value;
};


class serializable {
	virtual std::vector<serializableProperty> getProperties() = 0;
};

class myClass : public serializable{

	int myNumber;
	float myFloat;

	// usage somthing like this. Should expand to an implimentation of getProperties
	// PROPERTY_EXPORT({serializableType::INT, myNumber}, {serializableType::INT, myFloat});
};


class Player : public Behaviour {


public:

	BEHAVIOUR_CONSTUCTOR(Player) {
	
	}
	BEHAVIOUR_CLONE(Player)

	//std::string GetEditorName() override { // deprecate
	//	return "Player";
	//};

	//uint32_t getBehaviorHash() const override {
	//	return cHash("Player");
	//};


	// settings
	const float groundAccel = 30.0f;
	const float airAccel = 20.0f;
	const float topMoveSpeed = 7.0f; // max speed achieved through self movement
	const float terminalVelocity = 12.0f; // maximum speed achieved through gravity acceleration
	const float idleDecelleratiom = 45.0f;
	const float gravityAccel = 18;
	const float jumpVel = 6;
	const float skin = 0.01f;

	// variables
	vec2 velocity = vec2(0.0f);
	float grounded = true;

	float facingX = 1.0f;;


	bool queryTile(vec2 pos) {
		int tileX = pos.x / tileWorldSize + mapW / 2;
		int tileY = pos.y / tileWorldSize + mapH / 2;

		if (tileX > 0 && tileX < mapW && tileY > 0 && tileY < mapH) {

			tileY = mapH - tileY - 1;

			blockID block = tileWolrdGlobalRef->getTile(tileX, tileY);

			return block != Tiles::Air;
		}
		return false;
	}

	bool queryTile(ivec2 coord) {
		return tileWolrdGlobalRef->getTile(coord.x, coord.y);
	}

	ivec2 getTileXY(vec2 pos) {
		int tileX = pos.x / tileWorldSize + mapW / 2;
		int tileY = pos.y / tileWorldSize + mapH / 2;

		if (tileX > 0 && tileX < mapW && tileY > 0 && tileY < mapH)
			return ivec2(tileX, mapH - tileY - 1);
		return ivec2(-1, -1);
	}

	SpriteRenderer* renderer;

	vector<vec2> fanPoints;

	vector<vec2> tpoints;
	float sx = 0;
	float sy = 0;
	void Start() override {

		renderer = getComponent<SpriteRenderer>();
		renderer->atlasIndex = 5;

		tpoints.resize(10);
		float sx = transform->scale.x / 2.0;
		float sy = transform->scale.y / 2.0;
		tpoints[0] = vec2(sx, sy);
		tpoints[1] = vec2(-sx, sy);
		tpoints[2] = vec2(sx, -sy);
		tpoints[3] = vec2(-sx, -sy);
		tpoints[4] = vec2(-sx, sy / 2.0f);
		tpoints[5] = vec2(sx, sy / 2.0f);
		tpoints[6] = vec2(0, -sy);
		tpoints[7] = vec2(0, sy);
		tpoints[8] = vec2(-sx, -sy / 2.0f);
		tpoints[9] = vec2(sx, -sy / 2.0f);

		global::player = this;

		fanPoints = CreateTriangleFan(-0.5f, 0.5f, 4, 1.5f, vec2(0.0));
	};

	float animationTimer = 0;

	void Update() override {

		bool leftCast = false;
		bool rightCast = false;
		bool topCast = false;
		bool bottomCast = false;

		leftCast |= queryTile(transform->position + tpoints[1] + vec2(-skin * 2, 0));
		leftCast |= queryTile(transform->position + tpoints[3] + vec2(-skin * 2, 0));
		leftCast |= queryTile(transform->position + tpoints[4] + vec2(-skin * 2, 0));
		leftCast |= queryTile(transform->position + tpoints[8] + vec2(-skin * 2, 0));

		rightCast |= queryTile(transform->position + tpoints[0] + vec2(skin * 2, 0));
		rightCast |= queryTile(transform->position + tpoints[2] + vec2(skin * 2, 0));
		rightCast |= queryTile(transform->position + tpoints[5] + vec2(skin * 2, 0));
		rightCast |= queryTile(transform->position + tpoints[9] + vec2(skin * 2, 0));

		topCast |= queryTile(transform->position + tpoints[0] + vec2(0, skin * 2));
		topCast |= queryTile(transform->position + tpoints[1] + vec2(0, skin * 2));
		topCast |= queryTile(transform->position + tpoints[7] + vec2(0, skin * 2));

		bottomCast |= queryTile(transform->position + tpoints[2] + vec2(0, -skin * 2));
		bottomCast |= queryTile(transform->position + tpoints[3] + vec2(0, -skin * 2));
		bottomCast |= queryTile(transform->position + tpoints[6] + vec2(0, -skin * 2));

		animationTimer += deltaTime;
		if (animationTimer > 0.1f)
			animationTimer = 0.0f;

		//left and right movement
		bool left = input->getKey(KeyCode::LeftArrow) || input->getKey('a');
		bool right = input->getKey(KeyCode::RightArrow) || input->getKey('d');

		if (left && !right)
			facingX = -1;
		else if(!left &&  right)
			facingX = 1;

		if (left && !right && !leftCast) {
			velocity += vec2(grounded ? -groundAccel : -airAccel, 0.0f) * deltaTime;
		}
		else if (!left && right && !rightCast) {
			velocity += vec2(grounded ? groundAccel : airAccel, 0.0f) * deltaTime;
		}
		else if (!left && !right) {
			if (grounded) {
				if (velocity.x)
					velocity = decellerate(velocity, vec2(idleDecelleratiom * deltaTime, 0.0f));
			}
		}

		if ((input->getKeyDown(KeyCode::UpArrow) || input->getKey('w') || input->getKeyDown(KeyCode::Spacebar)) && grounded) {
			velocity.y = jumpVel;
			grounded = false;

		}
		else if (!bottomCast) {
			velocity.y -= gravityAccel * deltaTime;
		}


		if (grounded) {

			if (left && !right && !leftCast) {
				renderer->atlasIndex = animationTimer > 0.05 ? 1 : 2;
			}
			else if (!left && right && !rightCast) {
				renderer->atlasIndex = animationTimer > 0.05 ? 3 : 4;
			}
			else {
				renderer->atlasIndex = 5;
			}
		}
		else {
			renderer->atlasIndex = 0;
		}


		velocity.x = glm::clamp(velocity.x, -topMoveSpeed, topMoveSpeed);
		velocity.y = glm::clamp(velocity.y, -terminalVelocity, terminalVelocity);


		vec2 npos = transform->position + velocity * deltaTime;

		//int dx = velocity.x == 0 ? 0 : velocity.x < 0 ? -1 : 1;
		//int dy = velocity.y == 0 ? 0 : velocity.y < 0 ? -1 : 1;



		float resolvedXAvg = 0.0f;
		int avgSamplesX = 0;

		float resolvedYAvg = 0.0f;
		int avgSamplesY = 0;


		float slope = 0;

		float x1 = transform->position.x;
		float y1 = transform->position.y;
		float x2 = npos.x;
		float y2 = npos.y;

		if (x2 != x1) {
			slope = (y2 - y1) / (x2 - x1);
		}

		for (size_t i = 0; i < tpoints.size(); i++) {

			if (queryTile(npos + tpoints[i]) == false)
				continue;


			bool resolveLeft = false;
			bool resolveRight = false;
			bool resolveUp = false;
			bool resolveDown = false;

			volatile ivec2 prevTile = getTileXY(transform->position + tpoints[i]);
			volatile ivec2 curTile = getTileXY(npos + tpoints[i]);

			int dx = curTile.x - prevTile.x;
			int dy = -(curTile.y - prevTile.y);


			// test points on edge should be able to skip corner calculations if moving < 1 tile/fram
#if 1
			if (i > 3) {
				if (i == 4 || i == 8)
					resolveRight = true;
				else if (i == 5 || i == 9)
					resolveLeft = true;
				else if (i == 6)
					resolveUp = true;
				else if (i == 7)
					resolveDown = true;
			}
			else if (dx != 0 && dy == 0) {
#else
			if (dx != 0 && dy == 0) {
#endif
				if (dx > 0)
					resolveLeft = true;
				else
					resolveRight = true;
			}
			else if (dy != 0 && dx == 0) {
				if (dy > 0)
					resolveDown = true;
				else
					resolveUp = true;
			}
			else {
				if (dy == 0 && dx == 0) {
					continue;
				}
				assert(dy != 0 || dx != 0);
				assert(slope != 0);

				float left_x = (curTile.x - mapW / 2) * tileWorldSize - sx;
				float right_x = left_x + tileWorldSize;
				float bottom_y = (mapH / 2 - curTile.y - 1) * tileWorldSize - sy;
				float top_y = bottom_y + tileWorldSize;

				float left_y = slope * (left_x - x1) + y1;
				float right_y = slope * (right_x - x1) + y1;
				float bottom_x = (bottom_y - y1) / slope + x1;
				float top_x = (top_y - y1) / slope + x1;

				bool left_edge = dx == 1 && (bottom_y <= left_y && left_y <= top_y);
				bool right_edge = dx == -1 && (bottom_y <= right_y && right_y <= top_y);
				bool bottom_edge = dy == 1 && (left_x <= bottom_x && bottom_x <= right_x);
				bool top_edge = dy == -1 && (left_x <= top_x && top_x <= right_x);

				if (right_edge) {
					if (queryTile(ivec2(curTile.x + 1, curTile.y))) {
						if (dy < 0)
							resolveUp = true;
						else
							resolveDown = true;
					}
					else
						resolveRight = true;
				}
				else if (left_edge) {
					if (queryTile(ivec2(curTile.x - 1, curTile.y))) {
						if (dy < 0)
							resolveUp = true;
						else
							resolveDown = true;
					}
					else
						resolveLeft = true;
				}
				else if (bottom_edge) {
					if (queryTile(ivec2(curTile.x, curTile.y + 1))) {
						if (dx < 0)
							resolveRight = true;
						else
							resolveLeft = true;
					}
					else
						resolveDown = true;
				}
				else if (top_edge) {
					if (queryTile(ivec2(curTile.x, curTile.y - 1))) {
						if (dx < 0)
							resolveRight = true;
						else
							resolveLeft = true;
					}
					else
						resolveUp = true;
				}
			}

			if (resolveUp) {
				resolvedYAvg += (mapH / 2 - curTile.y) * tileWorldSize - tpoints[i].y + skin;
				avgSamplesY++;
				velocity.y = 0;
				grounded = true;
			}
			else if (resolveLeft) {
				resolvedXAvg += (curTile.x - mapW / 2) * tileWorldSize - tpoints[i].x - skin;
				avgSamplesX++;
				velocity.x = 0;
			}
			else if (resolveRight) {
				resolvedXAvg += (curTile.x - mapW / 2 + 1) * tileWorldSize - tpoints[i].x + skin;
				avgSamplesX++;
				velocity.x = 0;
			}
			else if (resolveDown) {
				resolvedYAvg += (mapH / 2 - curTile.y - 1) * tileWorldSize - tpoints[i].y - skin;
				avgSamplesY++;
				velocity.y = 0;
			}
		}

		if (avgSamplesX > 0)
			npos.x = resolvedXAvg / (float)avgSamplesX;
		if (avgSamplesY > 0)
			npos.y = resolvedYAvg / (float)avgSamplesY;

		transform->position = npos;

		if (transform->position != GcameraPos) {
			vec2 npos = GcameraPos + (transform->position - GcameraPos) * 4.0f * deltaTime;
			if (glm::distance(npos, transform->position) > glm::distance(GcameraPos, transform->position)) {
				GcameraPos = transform->position;
			}
			else {
				GcameraPos = npos;
			}
		}

		for (auto& p : fanPoints)
		{
			debugTriangles.push_back(p * facingX + transform->position);
		}

		if (input->getMouseBtnDown(MouseBtn::Left)) {

			std::vector<glm::vec2> swordTriangles;
			swordTriangles.reserve(fanPoints.size());
			for (auto& p : fanPoints)
			{
				swordTriangles.push_back(p * facingX + transform->position);
			}

			damageEnemiesTriangles(swordTriangles, 10.0f, 5);
		}
	};

	void damageEnemiesTriangles(std::vector<glm::vec2> vertices, float knockback, int damage);
};


#if 0


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
			resolvedXAvg += (curTile.x - mapW / 2) * tileWorldSize - sx - 0.0001f;
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

#endif