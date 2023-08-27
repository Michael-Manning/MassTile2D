
#pragma once

#include <unordered_map>
#include <string>
#include <set>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <memory>
#include <cassert>

#include <nlohmann/json.hpp>

#include "ECS.h"
#include "Physics.h"
#include "scene.h"
#include "texture.h"
#include "BehaviorRegistry.h"
#include "Input.h"

using namespace nlohmann;
using namespace std;

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

void Scene::SaveScene(std::string filename) {

	json j;
	for (auto& e : sceneData.entities) {
		if (e.second->persistent) {
			j["entities"].push_back(e.second->serializeJson());
		}
	}
	for (auto& s : sceneData.spriteRenderers) {
		if (sceneData.entities[s.first]->persistent) {
			j["spriteRenderers"].push_back(s.second.serializeJson(s.first));
		}
	}
	for (auto& c : sceneData.colorRenderers) {
		if (sceneData.entities[c.first]->persistent) {
			j["colorRenderers"].push_back(c.second.serializeJson(c.first));
		}
	}
	for (auto& r : sceneData.rigidbodies) {
		if (sceneData.entities[r.first]->persistent) {
			j["rigidbodies"].push_back(r.second.serializeJson(r.first));
		}
	}
	for (auto& s : sceneData.staticbodies) {
		if (sceneData.entities[s.first]->persistent) {
			j["staticbodies"].push_back(s.second.serializeJson(s.first));
		}
	}

	// assets
	auto usedSprites = sceneData.getUsedSprites();
	for (auto& s : usedSprites) {
		if (s != assetManager->defaultSprite)
			j["usedSprites"].push_back(s);
	}

	std::ofstream output(filename + ".json");
	output << j.dump(4) << std::endl;
	output.close();
}


void Scene::LoadScene(std::string filename, std::shared_ptr<b2World> world) {

	assert(world != nullptr);

	{
		sceneData.entities.clear();
		sceneData.colorRenderers.clear();
		sceneData.spriteRenderers.clear();

		for (auto& i : sceneData.staticbodies) {
			i.second.Destroy();
		}
		for (auto& i : sceneData.rigidbodies) {
			i.second.Destroy();
		}
		sceneData.staticbodies.clear();
		sceneData.rigidbodies.clear();
	}

	EntityGenerator.Reset();

	std::ifstream input(filename + ".json");

	json j;
	input >> j;

	set<spriteID> requiredSprites;
	for (auto& i : j["usedSprites"]) {
		requiredSprites.insert(static_cast<entityID>(i));
	}
	assetManager->loadSpriteAssets(requiredSprites);

	for (auto& i : j["entities"]) {
		shared_ptr<Entity> e = Entity::deserializeJson(i);
		e->persistent = true;
		OverwriteEntity(e, e->ID);
		//entities[e->ID] = e;
		EntityGenerator.Input(e->ID);
	}

	for (auto& i : j["colorRenderers"]) {
		entityID entID = i["entityID"];
		ColorRenderer r = ColorRenderer::deserializeJson(i);
		registerComponent(entID, r);
	}
	for (auto& i : j["spriteRenderers"]) {
		entityID entID = i["entityID"];
		SpriteRenderer r = SpriteRenderer::deserializeJson(i);
		registerComponent(entID, r);
	}
	for (auto& i : j["rigidbodies"]) {
		entityID entID = i["entityID"];
		Rigidbody r = Rigidbody::deserializeJson(i, world);
		registerComponent(entID, r);
	}
	for (auto& i : j["staticbodies"]) {
		entityID entID = i["entityID"];
		Staticbody r = Staticbody::deserializeJson(i, world);
		registerComponent(entID, r);
	}



	//for (auto& i : j["imgAssets"])
	//{
	//	//texID id = i["texID"];
	//	std::string src = i["src"];
	//	assetManager->loadTexture(src, true);
	//}

	assetManager->spritesAdded = true;
}

void Scene::UnregisterEntity(entityID id) {
	sceneData.entities.erase(id);
	sceneData.colorRenderers.erase(id);
	sceneData.spriteRenderers.erase(id);

	if (sceneData.rigidbodies.contains(id))
		sceneData.rigidbodies[id].Destroy();
	sceneData.rigidbodies.erase(id);

	if (sceneData.staticbodies.contains(id))
		sceneData.staticbodies[id].Destroy();
	sceneData.staticbodies.erase(id);
}

void Scene::RegisterEntity(std::shared_ptr<Entity> entity) {
	entityID id = EntityGenerator.GenerateID();
	entity->ID = id;
	if (entity->name.empty())
	{
		entity->name = string("entity ") + to_string(id);
		/*entity->name = "testName";*/
	}
	sceneData.entities[id] = entity;
	entity->_setComponentAccessor(componentAccessor);
}
void Scene::OverwriteEntity(std::shared_ptr<Entity> entity, entityID ID) {

	entity->ID = ID;
	if (entity->name.empty())
	{
		entity->name = string("entity ") + to_string(ID);
		/*entity->name = "testName";*/
	}
	sceneData.entities[ID] = entity;
	entity->_setComponentAccessor(componentAccessor);
}
void Scene::RegisterAsChild(std::shared_ptr<Entity> parent, std::shared_ptr<Entity> child) {
	RegisterEntity(child);
	parent->children.insert(child->ID);
	child->parent = parent->ID;
}

entityID Scene::DuplicateEntity(std::shared_ptr<Entity> entity) {
	std::shared_ptr<Entity> copy;
	if (entity->getBehaviorHash() == 0)
		copy = std::make_shared<Entity>();
	else
		copy = BehaviorMap[entity->getBehaviorHash()].second();

	copy->name = entity->name + std::string("_copy");
	copy->persistent = false;
	copy->transform = entity->transform;

	RegisterEntity(copy);

	if (sceneData.colorRenderers.contains(entity->ID)) {
		auto c = sceneData.colorRenderers[entity->ID].duplicate();
		registerComponent(copy, c);
	}
	if (sceneData.spriteRenderers.contains(entity->ID)) {
		auto c = sceneData.spriteRenderers[entity->ID].duplicate();
		registerComponent(copy, c);
	}
	if (sceneData.staticbodies.contains(entity->ID)) {
		auto c = sceneData.staticbodies[entity->ID].duplicate();
		registerComponent(copy, c);
	}
	if (sceneData.rigidbodies.contains(entity->ID)) {
		auto c = sceneData.rigidbodies[entity->ID].duplicate();
		registerComponent(copy, c);
	}

	return copy->ID;
}

Prefab Scene::CreatePrefab(std::shared_ptr<Entity> entity) {
	Prefab p;
	p.behaviorHash = entity->getBehaviorHash();

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

std::shared_ptr<Entity> Scene::Instantiate(Prefab& prefab, std::string name, glm::vec2 position, float rotation) {
	std::shared_ptr<Entity> copy;
	if (prefab.behaviorHash == 0)
		copy = std::make_shared<Entity>();
	else
		copy = BehaviorMap[prefab.behaviorHash].second();

	copy->name = name;
	copy->persistent = false;
	copy->transform = Transform(position, prefab.transform.scale, rotation);

	RegisterEntity(copy);

	if (prefab.colorRenderer.has_value()) {
		registerComponent(copy, prefab.colorRenderer.value());
	}
	if (prefab.spriteRenderer.has_value()) {
		registerComponent(copy, prefab.spriteRenderer.value());
	}
	if (prefab.staticbody.has_value()) {
		registerComponent(copy, prefab.staticbody.value());
	}
	if (prefab.rigidbody.has_value()) {
		registerComponent(copy, prefab.rigidbody.value());
	}

	return copy;
}






template <>
void Scene::registerComponent<SpriteRenderer>(entityID id, SpriteRenderer t) {
	sceneData.spriteRenderers[id] = t;
	assetManager->spritesAdded = true;
}


// Color Renderer
template <>
void Scene::registerComponent(entityID id, ColorRenderer t) {
	sceneData.colorRenderers[id] = t;
};



// Rigidbodoy
template <>
void Scene::registerComponent(entityID id, Rigidbody r) {
	const auto& t = sceneData.entities[id]->transform;
	if (r._bodyGenerated() == false) {
		r._generateBody(bworld, t.position, t.rotation);
	}
	else {
		r.SetTransform(t.position, t.rotation);
	}
	sceneData.rigidbodies.emplace(id, r);
};


// Staticbody
template <>
void Scene::registerComponent(entityID id, Staticbody s) {
	const auto& t = sceneData.entities[id]->transform;
	//s.body->SetTransform(gtb(t.position), t.rotation);
	s._generateBody(t.position, t.rotation);
	sceneData.staticbodies[id] = s;
};

