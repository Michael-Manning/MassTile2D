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


Scene::Scene() : bworld(gravity) {


	for (auto& [hash, stringPair] : BehaviorMap)
	{

		stringBehaviourMap.insert({ stringPair.first, std::pair<behavioiurHash, BehaviourFactoryFunc>(hash, stringPair.second) });
		//stringBehaviourMap.insert({ stringPair.first, stringPair.second });
	}

	componentAccessor = make_unique<ComponentAccessor>();
	componentAccessor->entities = &sceneData.entities;
	componentAccessor->colorRenderers = &sceneData.colorRenderers;
	componentAccessor->spriteRenderers = &sceneData.spriteRenderers;
	componentAccessor->textRenderers = &sceneData.textRenderers;
	componentAccessor->particleSystemRenderers = &sceneData.particleSystemRenderers;
	componentAccessor->rigidbodies = &sceneData.rigidbodies;
	componentAccessor->staticbodies = &sceneData.staticbodies;
}

void Scene::linkRigidbodyB2D(entityID id, Rigidbody* r) {
	const auto& t = sceneData.entities.at(id).transform;
	r->_LinkWorld(&bworld, t.position, t.rotation);
}

void Scene::linkStaticbodyB2D(entityID id, Staticbody* s) {
	const auto& t = sceneData.entities.at(id).transform;
	s->_LinkWorld(&bworld, t.position, t.rotation);
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


void Scene::deletechildRecurse(Entity* entity) {

	auto childCache = entity->_GetChildCache_ptr();
	for (auto& e : *childCache)
		deletechildRecurse(e);

	sceneData.entities.erase(entity->ID);
	sceneData.colorRenderers.erase(entity->ID);
	sceneData.spriteRenderers.erase(entity->ID);
	sceneData.particleSystemRenderers.erase(entity->ID);
	sceneData.textRenderers.erase(entity->ID);
	sceneData.rigidbodies.erase(entity->ID);
	sceneData.staticbodies.erase(entity->ID);
}

void Scene::DeleteEntity(entityID id, bool deleteChildren) {

	// NOTE: technically don't have to delete all the components right away.
	// Could clean up all the components at the end of each frame or n frames if it ends up being faster.

	Entity* entity = GetEntity(id);

	if (entity->HasParent())
		(*entity->_GetParentCache_ptr())->RemoveChild(entity);

	if (deleteChildren) {
		deletechildRecurse(entity);
	}
	else {

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
		sceneData.rigidbodies.erase(id);
		sceneData.staticbodies.erase(id);
	}
}

void Scene::ClearScene() {

	//for (auto& [ID, b] : sceneData.rigidbodies) 
	//	sceneData.rigidbodies.at(ID).Destroy();
	//for (auto& [ID, b] : sceneData.staticbodies)
	//	sceneData.staticbodies.at(ID).Destroy();

	sceneData.colorRenderers.clear();
	sceneData.spriteRenderers.clear();
	sceneData.textRenderers.clear();
	sceneData.particleSystemRenderers.clear();
	sceneData.staticbodies.clear();
	sceneData.rigidbodies.clear();

	sceneData.entities.clear();

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

	return entity;
}

Behaviour* Scene::AddBehaviour(Entity* entity, std::string behaviourName) {
	const auto& hashPtrPair = stringBehaviourMap.at(behaviourName);
	auto [iter, inserted] = sceneData.behaviours.emplace(entity->ID, hashPtrPair.second(hashPtrPair.first, entity));
	iter->second->_SetComponentAccessor(componentAccessor.get());
	return iter->second.get();
}
Behaviour* Scene::AddBehaviour(Entity* entity, behavioiurHash hash) {
	assert(BehaviorMap.contains(hash));
	auto [iter, inserted] = sceneData.behaviours.emplace(entity->ID, BehaviorMap.at(hash).second(hash, entity));
	iter->second->_SetComponentAccessor(componentAccessor.get());
	return iter->second.get();
}

void Scene::SetEntityAsChild(Entity* parent, Entity* child) {
	assert(child->HasParent() == false);
	parent->AddChild(child);
	child->SetParent(parent);
}

void Scene::gatherChildIDsRecurse(std::vector<entityID>& IDs, Entity* entity) {
	IDs.push_back(entity->ID);

	auto childCache = entity->_GetChildCache_ptr();
	for (auto& e : *childCache)
		gatherChildIDsRecurse(IDs, e);
}

Entity* Scene::DuplicateEntity(Entity* original) {

	vector<entityID> IDTree;
	gatherChildIDsRecurse(IDTree, original);

	std::unordered_map<entityID, entityID> IDRemap;
	for (auto& ID : IDTree)
	{
		IDRemap.insert({ ID, sceneData.EntityGenerator.GenerateID() });
	}


	Entity* copyTopLevel = nullptr;

	// set relationship IDs in new entities with mapped values from the originals
	for (auto& ID : IDTree) {

		Entity* e = GetEntity(ID);

		Entity* copy = sceneData.EmplaceEntity(IDRemap.at(ID));
		e->CloneInto(copy);

		copy->persistent = false;

		if (e == original) {
			copyTopLevel = copy;
			copy->ClearParent(); // cut off parent in copy
		}
		else if (e->HasParent())
		{
			*copy->_GetParent() = IDRemap.at(e->GetParent());
		}

		auto origChildSet = e->_GetChildSet();
		auto copyChildSet = copy->_GetChildSet();
		for (auto& child : *origChildSet) {
			copyChildSet->insert(IDRemap.at(child));
		}
	}

	for (auto& ID : IDTree) {

		if (sceneData.behaviours.contains(ID)) {
			entityID newID = IDRemap.at(ID);
			Entity* be = &sceneData.entities.at(newID);
			sceneData.behaviours.emplace(newID, sceneData.behaviours.at(ID)->clone(be));
		}

		// copy all components
		if (sceneData.colorRenderers.contains(ID))
			registerComponent(IDRemap.at(ID), sceneData.colorRenderers.at(ID));
		if (sceneData.spriteRenderers.contains(ID))
			registerComponent(IDRemap.at(ID), sceneData.spriteRenderers.at(ID));
		if (sceneData.textRenderers.contains(ID))
			registerComponent(IDRemap.at(ID), sceneData.textRenderers.at(ID));
		if (sceneData.particleSystemRenderers.contains(ID)) {
			auto& ps = sceneData.particleSystemRenderers.at(ID);
			registerComponent(IDRemap.at(ID), ps.size, ps.configuration);
		}
		if (sceneData.rigidbodies.contains(ID))
			registerComponent_Rigidbody(IDRemap.at(ID), sceneData.rigidbodies.at(ID));
		if (sceneData.staticbodies.contains(ID))
			registerComponent_Staticbody(IDRemap.at(ID), sceneData.staticbodies.at(ID));
	}

	LinkEntityRelationshipsRecurse(copyTopLevel);

	copyTopLevel->name = original->name + "(copy)";

	return copyTopLevel;
}


Entity* Scene::Instantiate(Prefab& prefab, std::string name, glm::vec2 position, float rotation) {

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

	for (auto& [ID, b] : prefab.sceneData.behaviours) {
		entityID newID = IDRemap.at(ID);
		Entity* be = &sceneData.entities.at(newID);
		sceneData.behaviours.emplace(newID, b->clone(be));
	}

	for (auto& [ID, r] : prefab.sceneData.colorRenderers)
		registerComponent(IDRemap.at(ID), r);
	for (auto& [ID, r] : prefab.sceneData.spriteRenderers)
		registerComponent(IDRemap.at(ID), r);
	for (auto& [ID, r] : prefab.sceneData.textRenderers)
		registerComponent(IDRemap.at(ID), r);
	for (auto& [ID, r] : prefab.sceneData.particleSystemRenderers)
		registerComponent(IDRemap.at(ID), r.size, r.configuration);
	for (auto& [ID, r] : prefab.sceneData.rigidbodies)
		registerComponent_Rigidbody(IDRemap.at(ID), r);
	for (auto& [ID, r] : prefab.sceneData.staticbodies)
		registerComponent_Staticbody(IDRemap.at(ID), r);

	assert(prefab.TopLevelEntity != NULL_Entity);

	Entity* topLevelEntity = &sceneData.entities.at(IDRemap.at(prefab.TopLevelEntity));
	topLevelEntity->transform = Transform(position, prefab.transform.scale, rotation);

	LinkEntityRelationshipsRecurse(topLevelEntity);

	return topLevelEntity;
}


void Scene::registerComponent(entityID id, ColorRenderer t) {
	sceneData.colorRenderers.insert({ id, t });
};

void Scene::registerComponent(entityID id, SpriteRenderer t) {
	sceneData.spriteRenderers.insert({ id, t });
}

void Scene::registerComponent(entityID id, TextRenderer t) {
	sceneData.textRenderers.insert({ id, t });
}

void Scene::registerComponent(entityID id, ParticleSystemRenderer::ParticleSystemSize systemSize) {
	sceneData.particleSystemRenderers.emplace(id, systemSize);
}
void Scene::registerComponent(entityID id, ParticleSystemRenderer::ParticleSystemSize systemSize, ParticleSystemPL::ParticleSystemConfiguration& configuration) {
	sceneData.particleSystemRenderers.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(id),
		std::forward_as_tuple(systemSize, configuration));
}



Rigidbody* Scene::registerComponent_Rigidbody(entityID id, const Collider& collider) {
	auto [iter, inserted] = sceneData.rigidbodies.emplace(id, collider);
	//const auto& t = sceneData.entities.at(id).transform;
	/*iter->second._LinkWorld(&bworld, t.position, t.rotation);*/
	linkRigidbodyB2D(id, &iter->second);
	return &iter->second;

};

Staticbody* Scene::registerComponent_Staticbody(entityID id, const Collider& collider) {
	auto [iter, inserted] = sceneData.staticbodies.emplace(id, collider);
	/*const auto& t = sceneData.entities.at(id).transform;
	iter->second._LinkWorld(&bworld, t.position, t.rotation);*/
	linkStaticbodyB2D(id, &iter->second);
	return &iter->second;
};

void Scene::registerComponent_Rigidbody(entityID id, const Rigidbody& body) {
	auto [iter, inserted] = sceneData.rigidbodies.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(id),
		std::forward_as_tuple(body, &bworld));

	const auto& t = sceneData.entities.at(id).transform;
	iter->second.SetTransform(t.position, t.rotation);
	//linkRigidbodyB2D(id, &iter->second);
};

void Scene::registerComponent_Staticbody(entityID id, const Staticbody& body) {
	auto [iter, inserted] = sceneData.staticbodies.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(id),
		std::forward_as_tuple(body, &bworld));

	const auto& t = sceneData.entities.at(id).transform;
	iter->second.SetTransform(t.position, t.rotation);
	//linkStaticbodyB2D(id, &iter->second);
};

void Scene::cleanup() {

	// ensure desctructors are called
	sceneData.rigidbodies.clear();
	sceneData.staticbodies.clear();

	for (b2Body* body = bworld.GetBodyList(); body != nullptr; ) {
		b2Body* nextBody = body->GetNext();

		// Destroy joints attached to the body
		for (b2JointEdge* j = body->GetJointList(); j; j = j->next) {
			bworld.DestroyJoint(j->joint);
		}

		// Now destroy the body
		bworld.DestroyBody(body);
		body = nextBody; // Move to the next body in the list
	}
}


#ifdef USING_EDITOR

#include <regex>

std::string Scene::GetNoneConflictingEntityName(Entity* entity, Entity* hierarchyLevel)
{

	std::string name = entity->name;

	robin_hood::unordered_flat_set<std::string> usedNames;

	if (hierarchyLevel != nullptr) {
		auto childPtr = hierarchyLevel->_GetChildCache_ptr();
		for (auto& c : *childPtr)
		{
			if (c == entity)
				continue;
			usedNames.insert(c->name);
		}
	}
	else {
		for (auto& [ID, e] : sceneData.entities)
		{
			if (&e == entity)
				continue;

			if (e.HasParent() == false)
				usedNames.insert(e.name);
		}
	}

	if (usedNames.contains(name) == false)
		return name;

	// Prepare to check if the name ends with "(n)" and extract the base name and the number.
	std::regex namePattern(R"((.*)(\(\d+\))$)");
	std::smatch matches;

	std::string baseName = name; // The original name or the name without the trailing number.
	int number = 1; // The starting number to append or increment.

	// If the name already ends with "(n)", extract the base name and start with the next number.
	if (std::regex_match(name, matches, namePattern)) {
		if (matches.size() == 3) { // matches[0] is the full match, [1] is the base name, [2] is the number part.
			baseName = matches[1].str();
			// Extract the number, ignoring the surrounding parentheses.
			number = std::stoi(matches[2].str().substr(1, matches[2].str().size() - 2)) + 1;
		}
	}

	// Generate new names until one is found that isn't used.
	std::string newName;
	do {
		newName = baseName + "(" + std::to_string(number) + ")";
		number++; // Prepare for the next iteration, if necessary.
	} while (usedNames.find(newName) != usedNames.end());

	return newName;
}


#endif