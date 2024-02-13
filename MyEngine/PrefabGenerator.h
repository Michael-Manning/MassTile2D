#pragma once

#include "Scene.h"
#include "ECS.h"
#include "Prefab.h"

Prefab GeneratePrefab(Entity* entity, SceneData& sceneData) {
	Prefab p;
	p.behaviorHash = entity->getBehaviorHash();

	p.name = entity->name;

	if (sceneData.colorRenderers.contains(entity->ID))
		p.colorRenderer = sceneData.colorRenderers[entity->ID];
	if (sceneData.spriteRenderers.contains(entity->ID))
		p.spriteRenderer = sceneData.spriteRenderers[entity->ID];
	if (sceneData.staticbodies.contains(entity->ID))
		p.staticbody = sceneData.staticbodies[entity->ID];
	if (sceneData.rigidbodies.contains(entity->ID))
		p.rigidbody = sceneData.rigidbodies[entity->ID];

	return p;
}
