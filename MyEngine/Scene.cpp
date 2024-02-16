#include "stdafx.h"

#include <unordered_map>
#include <string>
#include <set>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <memory>
#include <cassert>

#include <nlohmann/json.hpp>
#include <robin_hood.h>

#include "ECS.h"
#include "Physics.h"
#include "scene.h"
#include "texture.h"
#include "BehaviorRegistry.h"
#include "Input.h"

#include <assetPack/Scene_generated.h>

using namespace nlohmann;
using namespace std;

void Scene::linkRigidbodyB2D(entityID id, Rigidbody* r) {
	const auto& t = sceneData.entities.at(id).transform;
	r->_generateBody(&bworld, t.position, t.rotation);
}

void Scene::linkStaticbodyB2D(entityID id, Staticbody* s) {
	const auto& t = sceneData.entities.at(id).transform;
	s->_generateBody(&bworld, t.position, t.rotation);
}

void Scene::CreateComponentAccessor() {
	auto getColorRenderer = [this](entityID id) {
		assert(sceneData.colorRenderers.contains(id));
		return &sceneData.colorRenderers[id];
		};

	auto getSpriteRenderer = [this](entityID id) {
		assert(sceneData.spriteRenderers.contains(id));
		return &sceneData.spriteRenderers[id];
		};

	auto getStaticbody = [this](entityID id) {
		assert(sceneData.staticbodies.contains(id));
		return &sceneData.staticbodies[id];
		};

	auto getRigidbody = [this](entityID id) {
		assert(sceneData.rigidbodies.contains(id));
		return &sceneData.rigidbodies[id];
		};

	componentAccessor = make_shared<ComponentAccessor>(
		getColorRenderer,
		getSpriteRenderer,
		getStaticbody,
		getRigidbody
	);
}

void Scene::serializeJson(std::string filename) {
	json j;

	j["name"] = name;

	j["sceneData"] = sceneData.serializeJson();

	checkAppend(filename, Scene_extension);
	std::ofstream output(filename);
	output << j.dump(4) << std::endl;
	output.close();
}

void Scene::LinkEntityRelationshipsRecurse(Entity* entity) {
	auto children_ptr = entity->_GetChildCache_ptr();
	auto children = entity->_GetChildSet();

	for (entityID& childID : *children)
	{
		Entity* child = &sceneData.entities.at(childID);
		*child->_GetParentCache_ptr() = entity;
		children_ptr->insert(child);
		LinkEntityRelationshipsRecurse(child);
	}
}

void Scene::LinkEntityRelationships() {
	for (auto& [id, entity] : sceneData.entities)
	{
		if (entity.HasParent() == false && entity.HasChildren() == true) {
			LinkEntityRelationshipsRecurse(&entity);
		}
	}
}

std::shared_ptr<Scene> Scene::deserializeJson(std::string filename) {

	checkAppend(filename, ".scene");
	std::ifstream input(filename);
	json j;
	input >> j;

	auto scene = std::make_shared<Scene>();

	scene->name = j["name"];

	SceneData::deserializeJson(j["sceneData"], &scene->sceneData);

	for (auto& [id, r] : scene->sceneData.rigidbodies)
		scene->linkRigidbodyB2D(id, &r);
	for (auto& [id, r] : scene->sceneData.staticbodies)
		scene->linkStaticbodyB2D(id, &r);

	scene->LinkEntityRelationships();
	return scene;
}


std::shared_ptr<Scene> Scene::deserializeFlatbuffers(const AssetPack::Scene* s) {
	auto scene = std::make_shared<Scene>();

	scene->name = s->name()->str();

	SceneData::deserializeFlatbuffers(s->sceneData(), &scene->sceneData);

	for (auto& [id, r] : scene->sceneData.rigidbodies)
		scene->linkRigidbodyB2D(id, &r);
	for (auto& [id, r] : scene->sceneData.staticbodies)
		scene->linkStaticbodyB2D(id, &r);

	scene->LinkEntityRelationships();
	return scene;
}


void Scene::UnregisterEntity(entityID id) {

	// NOTE: technically don't have to delete all the components right away.
	// Could clean up all the components at the end of each frame or n frames if it ends up being faster.

	auto entity = GetEntity(id);
	if (entity->HasParent())
		(*entity->_GetParentCache_ptr())->RemoveChild(entity);
	if (entity->HasChildren()) {
		auto cache = entity->_GetChildCache_ptr();
		for (auto& child : *cache) {
			child->ClearParent();
		}
	}

	sceneData.entities.erase(id);
	sceneData.colorRenderers.erase(id);
	sceneData.spriteRenderers.erase(id);
	sceneData.particleSystemRenderers.erase(id);
	sceneData.textRenderers.erase(id);

	if (sceneData.rigidbodies.contains(id))
		sceneData.rigidbodies[id].Destroy();
	sceneData.rigidbodies.erase(id);

	if (sceneData.staticbodies.contains(id))
		sceneData.staticbodies[id].Destroy();
	sceneData.staticbodies.erase(id);
}


Entity* Scene::CreateEntity(Transform transform, std::string name, bool persistent) {
	entityID id = sceneData.EntityGenerator.GenerateID();

	sceneData.entities.insert(robin_hood::pair<const entityID, Entity>(id, Entity(name, persistent)));
	Entity* entity = &sceneData.entities.at(id);

	entity->ID = id;
	entity->transform = transform;

	if (entity->name.empty()) {
		entity->name = string("entity ") + to_string(id);
	}
	entity->_setComponentAccessor(componentAccessor);
	return entity;
}

//Entity* Scene::EmplaceEntity(entityID ID) {
//	if (EntityGenerator.Contains(ID) == false) {
//		// new entity
//		EntityGenerator.Input(ID);
//		auto [iterator, inserted] = sceneData.entities.emplace(std::piecewise_construct, std::forward_as_tuple(ID), std::tuple<>());
//		return &iterator->second;
//	}
//	else {
//		// overwrite entity
//		sceneData.entities.insert_or_assign(ID, Entity());
//		return GetEntity(ID);
//	}
//}


void Scene::SetEntityAsChild(Entity* parent, Entity* child) {
	assert(child->HasParent() == false);
	parent->AddChild(child);
	child->SetParent(parent);
}

entityID Scene::DuplicateEntity(entityID original) {
	assert(false);
	return 0;

	//std::shared_ptr<Entity> copy;
	//if (entity->getBehaviorHash() == 0)
	//	copy = std::make_shared<Entity>();
	//else
	//	copy = BehaviorMap[entity->getBehaviorHash()].second();

	//copy->name = entity->name + std::string("_copy");
	//copy->persistent = false;
	//copy->transform = entity->transform;

	//RegisterEntity(copy);

	//if (sceneData.colorRenderers.contains(entity->ID)) {
	//	auto c = sceneData.colorRenderers[entity->ID].duplicate();
	//	registerComponent(copy, c);
	//}
	//if (sceneData.spriteRenderers.contains(entity->ID)) {
	//	auto c = sceneData.spriteRenderers[entity->ID].duplicate();
	//	registerComponent(copy, c);
	//}
	//if (sceneData.textRenderers.contains(entity->ID)) {
	//	auto c = sceneData.textRenderers[entity->ID].duplicate();
	//	registerComponent(copy, c);
	//}
	//if (sceneData.staticbodies.contains(entity->ID)) {
	//	auto c = sceneData.staticbodies[entity->ID].duplicate();
	//	registerComponent(copy, c);
	//}
	//if (sceneData.rigidbodies.contains(entity->ID)) {
	//	auto c = sceneData.rigidbodies[entity->ID].duplicate();
	//	registerComponent(copy, c);
	//}

	//return copy->ID;
}

//Prefab Scene::CreatePrefab(std::shared_ptr<Entity> entity) {
//	Prefab p;
//	p.behaviorHash = entity->getBehaviorHash();
//
//	if (sceneData.colorRenderers.contains(entity->ID))
//		p.colorRenderer = sceneData.colorRenderers[entity->ID];
//	if (sceneData.spriteRenderers.contains(entity->ID))
//		p.spriteRenderer = sceneData.spriteRenderers[entity->ID];
//	if (sceneData.staticbodies.contains(entity->ID))
//		p.staticbody = sceneData.staticbodies[entity->ID];
//	if (sceneData.rigidbodies.contains(entity->ID))
//		p.rigidbody = sceneData.rigidbodies[entity->ID];
//
//	return p;
//}

Entity* Scene::Instantiate(Prefab& prefab, std::string name, glm::vec2 position, float rotation) {
	//std::shared_ptr<Entity> copy;
	//if (prefab.behaviorHash == 0)
	//	copy = std::make_shared<Entity>();
	//else
	//	copy = BehaviorMap[prefab.behaviorHash].second();

	if (prefab.behaviorHash != 0) {
		assert(false);
	}

	std::unordered_map<entityID, entityID> IDRemap;
	for (auto& [ID, e] : prefab.sceneData.entities)
	{
		IDRemap.insert({ ID, sceneData.EntityGenerator.GenerateID() });
	}


	// set relationship IDs in new entities with mapped values from the originals
	for (auto& [ID, e] : prefab.sceneData.entities) {
		Entity* copy = sceneData.EmplaceEntity(IDRemap.at(ID));
		e.CloneInto(copy);

		copy->persistent = false;

		if (e.HasParent())
			*copy->_GetParent() = IDRemap.at(e.GetParent());


		auto origChildSet = e._GetChildSet();
		auto copyChildSet = copy->_GetChildSet();
		for (auto& child : *origChildSet) {
			copyChildSet->insert(IDRemap.at(child));
		}
	}

	for (auto& [ID, r] : prefab.sceneData.colorRenderers)
		registerComponent(IDRemap.at(ID), r);
	for (auto& [ID, r] : prefab.sceneData.spriteRenderers)
		registerComponent(IDRemap.at(ID), r);
	for (auto& [ID, r] : prefab.sceneData.rigidbodies)
		registerComponent(IDRemap.at(ID), r);
	for (auto& [ID, r] : prefab.sceneData.staticbodies)
		registerComponent(IDRemap.at(ID), r);

	assert(prefab.TopLevelEntity != NULL_Entity);

	Entity* topLevelEntity = &sceneData.entities.at(IDRemap.at(prefab.TopLevelEntity));
	topLevelEntity->transform = Transform(position, prefab.transform.scale, rotation);

	LinkEntityRelationshipsRecurse(topLevelEntity);

	return topLevelEntity;
}

// I don't think these need to be templates

template <>
void Scene::registerComponent(entityID id, ColorRenderer t) {
	sceneData.colorRenderers.insert({ id, t });
};

template <>
void Scene::registerComponent<SpriteRenderer>(entityID id, SpriteRenderer t) {
	sceneData.spriteRenderers.insert({ id, t });
}

template <>
void Scene::registerComponent<TextRenderer>(entityID id, TextRenderer t) {
	sceneData.textRenderers.insert({ id, t });
}

void Scene::registerComponent_ParticleSystem(entityID id, ParticleSystemRenderer::ParticleSystemSize systemSize) {
	sceneData.particleSystemRenderers.emplace(id, systemSize);
}

//template <>
//void Scene::registerComponent<ParticleSystemRenderer>(entityID id, ParticleSystemRenderer component) {
//	sceneData.particleSystemRenderers[id] = component;
//}

// Rigidbodoy
template <>
void Scene::registerComponent(entityID id, Rigidbody r) {
	auto [iter, inserted] = sceneData.rigidbodies.emplace(id, r);
	linkRigidbodyB2D(id, &iter->second);
};

// Staticbody
template <>
void Scene::registerComponent(entityID id, Staticbody s) {
	auto [iter, inserted] = sceneData.staticbodies.emplace(id, s);
	linkStaticbodyB2D(id, &iter->second);
};

