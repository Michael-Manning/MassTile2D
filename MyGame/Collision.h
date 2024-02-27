#pragma once

#include <vector>

#include <glm/glm.hpp>


struct ColliderCallback {
	std::vector<glm::vec2>* pointList;
	std::function<void(int damage, float knockback, glm::vec2 hitOrigin)> OnCollision;
};

bool isPointInTriangle(const glm::vec2& v1, const glm::vec2& v2, const glm::vec2& v3, const glm::vec2& p);
bool isPointInTriangles(const std::vector<glm::vec2>& trianglePoints, const glm::vec2& p);

void AddToEnemyHitCheckList(ColliderCallback* callback);
void RemoveFromEnemyHitCheckList(ColliderCallback* callback);

std::vector<glm::vec2> CreateTriangleFan(float startAngle, float endAngle, int count, float radius, glm::vec2 origin);

extern robin_hood::unordered_flat_set<ColliderCallback*> enemyColliderList;

extern std::vector<glm::vec2> debugTriangles;