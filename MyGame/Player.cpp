#include "stdafx.h"

#include "Player.h"
#include "Collision.h"


using namespace std;
using namespace glm;

std::unique_ptr<Tool> heldTool = nullptr;

void Player::damageEnemiesTriangles(std::vector<glm::vec2> vertices, float knockback, int damage) {

	for (auto& callback : enemyColliderList)
	{
		for (auto& p : *callback->pointList)
		{
			if (isPointInTriangles(vertices, p)) {
				callback->OnCollision(damage, knockback, transform->position);
				cout << "callback\n";
				break;
			}
		}
	}

}