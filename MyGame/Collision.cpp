#include "stdafx.h"


#include <glm/glm.hpp>

#include <robin_hood.h>
#include "Collision.h"

namespace {
	float mypi = glm::pi<float>();

}
	std::vector<glm::vec2> debugTriangles;
robin_hood::unordered_flat_set<ColliderCallback*> enemyColliderList;


using namespace std;
using namespace glm;

bool isPointInTriangle(const glm::vec2& v1, const glm::vec2& v2, const glm::vec2& v3, const glm::vec2& p) {
	auto sign = [](const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
		return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
		};

	bool b1, b2, b3;

	b1 = sign(p, v1, v2) < 0.0f;
	b2 = sign(p, v2, v3) < 0.0f;
	b3 = sign(p, v3, v1) < 0.0f;

	return ((b1 == b2) && (b2 == b3));
}

bool isPointInTriangles(const std::vector<glm::vec2>& trianglePoints, const glm::vec2& p) {
	assert(trianglePoints.size() % 3 == 0);

	for (size_t i = 0; i < trianglePoints.size() / 3; i++)
	{
		if (isPointInTriangle(
			trianglePoints[i * 3 + 0],
			trianglePoints[i * 3 + 1],
			trianglePoints[i * 3 + 2],
			p
		)) {
			return true;
		}

	}
	return false;
}

void AddToEnemyHitCheckList(ColliderCallback* callback) {
	enemyColliderList.insert(callback);
}

void RemoveFromEnemyHitCheckList(ColliderCallback* callback) {
	enemyColliderList.erase(callback);
}



float normalizeAngle(float angle) {
	angle = fmodf(angle, 2 * mypi); // Handle overflow and underflow
	if (angle < 0) {
		angle += 2 * mypi; // Adjust if negative
	}
	return angle;
}

// Calculate the directional difference between two angles
float angleDifference(float startAngle, float endAngle) {
	// Normalize both angles
	startAngle = normalizeAngle(startAngle);
	endAngle = normalizeAngle(endAngle);

	// Calculate the difference
	float difference = endAngle - startAngle;

	// Adjust for rollover
	if (difference > mypi) {
		difference -= 2 * mypi; // Clockwise is shorter
	}
	else if (difference < -mypi) {
		difference += 2 * mypi; // Counter-clockwise is shorter
	}

	return difference;
}

std::vector<glm::vec2> CreateTriangleFan(float startAngle, float endAngle, int count, float radius, glm::vec2 origin) {
	std::vector<glm::vec2> vertices;

	
	assert(count >= 1);

	
	float range = angleDifference(startAngle, endAngle);
	
	float step = range / (float)(count);

	

	glm::vec2 coolPoint{
		 origin.x + radius * cos(startAngle),
		 origin.y + radius * sin(startAngle)
	};

	for (int i = 1; i < count + 1; ++i) {
	
		float angle = startAngle + i * step;
	
		float x = origin.x + radius * cos(angle);
		float y = origin.y + radius * sin(angle);

		glm::vec2 coolPoint2{
			origin.x + radius * cos(angle),
			origin.y + radius * sin(angle)
		};

		vertices.push_back(origin);
		vertices.push_back(coolPoint);
		vertices.push_back(coolPoint2);

		coolPoint = coolPoint2;
	}

	return vertices;
}