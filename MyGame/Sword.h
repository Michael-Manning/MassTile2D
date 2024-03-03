#pragma once

#include "Tool.h"
#include "Collision.h"
#include "Input.h"
#include "global.h"

class Sword : public Tool {

private:
	std::vector <glm::vec2 > fanPoints;

public:
	Sword() {
		fanPoints = CreateTriangleFan(-0.5f, 0.5f, 4, 1.5f, glm::vec2(0.0));
	}

	void Update(const PlayerCtx& ctx) override {

		for (auto& p : fanPoints)
		{
			debugTriangles.push_back(p * ctx.facingX + ctx.position);
		}

		if (global::input->getMouseBtnDown(MouseBtn::Left)) {

			std::vector<glm::vec2> swordTriangles;
			swordTriangles.reserve(fanPoints.size());
			for (auto& p : fanPoints)
			{
				swordTriangles.push_back(p * ctx.facingX + ctx.position);
			}

			for (auto& callback : enemyColliderList)
			{
				for (auto& p : *callback->pointList)
				{
					if (isPointInTriangles(swordTriangles, p)) {
						callback->OnCollision(5, 10.0, ctx.position);
						break;
					}
				}
			}
		}
	}
};