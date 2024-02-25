
#include "stdafx.h"

#include "Scene.h"
#include "global.h"
#include "AssetManager.h"

#include "demon.h"



void Demon::OnCollision(int damage, float knockback, glm::vec2 hitOrigin) {
	/*auto notrmvec = glm::normalize(transform->position - hitOrigin);*/
	//velocity += glm::normalize(transform->position - hitOrigin) * knockback;
	if (transform->position.x > hitOrigin.x) {
		velocity.x += knockback;
	}
	else
		velocity.x -= knockback;

	health -= damage;

	if (health <= 0) {
		auto blood = global::mainScene->Instantiate(global::assetManager->GetPrefab("blood_burst"), "blood", transform->position);
		global::mainScene->DeleteAfter(blood, 2.0f);
		Behaviour::Destory();
	}


}