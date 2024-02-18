#include "stdafx.h"

#include <vector>

#include <robin_hood.h>

#include "Scene.h"
#include "ECS.h"
#include "Prefab.h"
#include"typedefs.h"

#include "PrefabGenerator.h"

using namespace std;

namespace {

	// fill child entity hierarchry 
	void recurseChildren(vector<Entity*>& entityList, Entity* entity) {

		entityList.push_back(entity);

		auto cache = entity->_GetChildCache_ptr();
		for (auto& child : *cache){
			recurseChildren(entityList, child);
		}
	}
}


void GeneratePrefab(Entity* entity, const SceneData& sceneData, Prefab* p) {

	vector<Entity*> entityList;
	recurseChildren(entityList, entity);

	p->name = entity->name;

	for (auto& e : entityList)
	{
		p->sceneData.entities.insert({ e->ID, *e });

		if (sceneData.colorRenderers.contains(e->ID))
			p->sceneData.colorRenderers.insert({ e->ID, sceneData.colorRenderers.at(e->ID)});
		if (sceneData.spriteRenderers.contains(e->ID))
			p->sceneData.spriteRenderers.insert({ e->ID, sceneData.spriteRenderers.at(e->ID) });
		if(sceneData.textRenderers.contains(e->ID))
			p->sceneData.textRenderers.insert({ e->ID, sceneData.textRenderers.at(e->ID) });
		if (sceneData.particleSystemRenderers.contains(e->ID)) {
			auto pr = &sceneData.particleSystemRenderers.at(e->ID);

			p->sceneData.particleSystemRenderers.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(e->ID),
				std::forward_as_tuple(pr->size, pr->configuration));
		}
		if (sceneData.staticbodies.contains(e->ID))
			p->sceneData.staticbodies.insert({ e->ID, sceneData.staticbodies.at(e->ID) });
		if (sceneData.rigidbodies.contains(e->ID))
			p->sceneData.rigidbodies.insert({ e->ID, sceneData.rigidbodies.at(e->ID) });
	}

	// should clear parent status of top level to remove potential hanging refernce
	p->sceneData.entities.at(entity->ID).ClearParent();

	p->TopLevelEntity = entity->ID;
}
