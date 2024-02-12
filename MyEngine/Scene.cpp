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

#include "ECS.h"
#include "Physics.h"
#include "scene.h"
#include "texture.h"
#include "BehaviorRegistry.h"
#include "Input.h"

#include <assetPack/Scene_generated.h>

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

void Scene::serializeJson(std::string filename) {
	json j;

	j["name"] = name;

	for (auto& [id, e] : sceneData.entities) {
		if (e.persistent) {
			j["entities"].push_back(e.serializeJson());
		}
	}
	for (auto& s : sceneData.spriteRenderers) {
		if (sceneData.entities.at(s.first).persistent) {
			j["spriteRenderers"].push_back(s.second.serializeJson(s.first));
		}
	}
	for (auto& c : sceneData.colorRenderers) {
		if (sceneData.entities.at(c.first).persistent) {
			j["colorRenderers"].push_back(c.second.serializeJson(c.first));
		}
	}
	for (auto& c : sceneData.textRenderers) {
		if (sceneData.entities.at(c.first).persistent) {
			j["textRenderers"].push_back(c.second.serializeJson(c.first));
		}
	}
	for (auto& r : sceneData.rigidbodies) {
		if (sceneData.entities.at(r.first).persistent) {
			j["rigidbodies"].push_back(r.second.serializeJson(r.first));
		}
	}
	for (auto& s : sceneData.staticbodies) {
		if (sceneData.entities.at(s.first).persistent) {
			j["staticbodies"].push_back(s.second.serializeJson(s.first));
		}
	}

	// assets
	auto usedSprites = sceneData.getUsedSprites();
	for (auto& s : usedSprites) {
		j["usedSprites"].push_back(s);
	}
	auto usedFonts = sceneData.getUsedFonts();
	for (auto& f : usedFonts) {
		j["usedFonts"].push_back(f);
	}
	checkAppend(filename, ".scene");
	std::ofstream output(filename);
	output << j.dump(4) << std::endl;
	output.close();
}

std::shared_ptr<Scene> Scene::deserializeJson(std::string filename) {

	checkAppend(filename, ".scene");
	std::ifstream input(filename);
	json j;
	input >> j;

	auto scene = std::make_shared<Scene>();

	scene->name = j["name"];

	for (auto& i : j["entities"]) {
		shared_ptr<Entity> e = Entity::deserializeJson(i);
		e->persistent = true;
		scene->OverwriteEntity(e, e->ID);
		scene->EntityGenerator.Input(e->ID);
	}
	for (auto& i : j["colorRenderers"]) {
		entityID entID = i["entityID"];
		ColorRenderer r = ColorRenderer::deserializeJson(i);
		scene->registerComponent(entID, r);
	}
	for (auto& i : j["spriteRenderers"]) {
		entityID entID = i["entityID"];
		SpriteRenderer r = SpriteRenderer::deserializeJson(i);
		scene->registerComponent(entID, r);
	}
	for (auto& i : j["textRenderers"]) {
		entityID entID = i["entityID"];
		TextRenderer r = TextRenderer::deserializeJson(i);
		scene->registerComponent(entID, r);
	}
	for (auto& i : j["rigidbodies"]) {
		entityID entID = i["entityID"];
		Rigidbody r = Rigidbody::deserializeJson(i);
		scene->registerComponent(entID, r);
	}
	for (auto& i : j["staticbodies"]) {
		entityID entID = i["entityID"];
		Staticbody r = Staticbody::deserializeJson(i);
		scene->registerComponent(entID, r);
	}

	return scene;
}


std::shared_ptr<Scene> Scene::deserializeFlatbuffers(const AssetPack::Scene * s) {
	auto scene = std::make_shared<Scene>();

	scene->name = s->name()->str();

	for (size_t i = 0; i < s->entities()->size(); i++) {
		shared_ptr<Entity> e = Entity::deserializeFlatbuffers(s->entities()->Get(i));
		e->persistent = true;
		scene->OverwriteEntity(e, e->ID);
		scene->EntityGenerator.Input(e->ID);
	}
	for (size_t i = 0; i < s->colorRenderers()->size(); i++) {
		ColorRenderer r = ColorRenderer::deserializeFlatbuffers(s->colorRenderers()->Get(i));
		entityID entID = s->colorRenderers()->Get(i)->entityID();
		scene->registerComponent(entID, r);
	}
	for (size_t i = 0; i < s->spriteRenderers()->size(); i++) {
		SpriteRenderer r = SpriteRenderer::deserializeFlatbuffers(s->spriteRenderers()->Get(i));
		entityID entID = s->spriteRenderers()->Get(i)->entityID();
		scene->registerComponent(entID, r);
	}
	for (size_t i = 0; i < s->textRenderers()->size(); i++) {
		TextRenderer r = TextRenderer::deserializeFlatbuffers(s->textRenderers()->Get(i));
		entityID entID = s->textRenderers()->Get(i)->entityID();
		scene->registerComponent(entID, r);
	}
	for (size_t i = 0; i < s->rigidbodies()->size(); i++) {
		Rigidbody r = Rigidbody::deserializeFlatbuffers(s->rigidbodies()->Get(i));
		entityID entID = s->rigidbodies()->Get(i)->entityID();
		scene->registerComponent(entID, r);
	}
	for (size_t i = 0; i < s->staticbodies()->size(); i++) {
		Staticbody r = Staticbody::deserializeFlatbuffers(s->staticbodies()->Get(i));
		entityID entID = s->staticbodies()->Get(i)->entityID();
		scene->registerComponent(entID, r);
	}

	return scene;
}


void Scene::UnregisterEntity(entityID id) {
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

//entityID  Scene::RegisterEntity(std::shared_ptr<Entity> entity) {
//	entityID id = EntityGenerator.GenerateID();
//	entity->ID = id;
//	if (entity->name.empty()) {
//		entity->name = string("entity ") + to_string(id);
//	}
//	sceneData.entities[id] = entity;
//	entity->_setComponentAccessor(componentAccessor);
//	return id;
//}

Entity* Scene::CreateEntity(Transform transform, std::string name) {
	entityID id = EntityGenerator.GenerateID();

	sceneData.entities.insert(std::pair<entityID, Entity>(id, Entity(name)));
	Entity* entity = &sceneData.entities.at(id);
	
	entity->ID = id;
	entity->transform = transform;

	if (entity->name.empty()) {
		entity->name = string("entity ") + to_string(id);
	}
	entity->_setComponentAccessor(componentAccessor);
	return entity;
}


void Scene::OverwriteEntity(std::shared_ptr<Entity> entity, entityID ID) {
	entity->ID = ID;
	if (entity->name.empty()) {
		entity->name = string("entity ") + to_string(ID);
	}
	sceneData.entities[ID] = entity;
	entity->_setComponentAccessor(componentAccessor);
}
void Scene::RegisterAsChild(std::shared_ptr<Entity> parent, std::shared_ptr<Entity> child) {
	assert(false);
	//RegisterEntity(child);
	//parent->children.insert(child->ID);
	//child->parent = parent->ID;
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
	if (sceneData.textRenderers.contains(entity->ID)) {
		auto c = sceneData.textRenderers[entity->ID].duplicate();
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

std::shared_ptr<Entity> Scene::Instantiate(Prefab prefab, std::string name, glm::vec2 position, float rotation) {
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
	if (prefab.textRenderer.has_value()) {
		registerComponent(copy, prefab.textRenderer.value());
	}
	if (prefab.staticbody.has_value()) {
		registerComponent(copy, prefab.staticbody.value());
	}
	if (prefab.rigidbody.has_value()) {
		registerComponent(copy, prefab.rigidbody.value());
	}

	return copy;
}

// I don't think these need to be templates

template <>
void Scene::registerComponent(entityID id, ColorRenderer t) {
	sceneData.colorRenderers[id] = t;
};

template <>
void Scene::registerComponent<SpriteRenderer>(entityID id, SpriteRenderer t) {
	sceneData.spriteRenderers[id] = t;
}

template <>
void Scene::registerComponent<TextRenderer>(entityID id, TextRenderer t) {
	sceneData.textRenderers[id] = t;
}

void Scene::registerComponent_ParticleSystem(entityID id, ParticleSystemRenderer::ParticleSystemSize systemSize) {
	sceneData.particleSystemRenderers.insert(std::pair<entityID, ParticleSystemRenderer>(id, ParticleSystemRenderer(systemSize)));
}

//template <>
//void Scene::registerComponent<ParticleSystemRenderer>(entityID id, ParticleSystemRenderer component) {
//	sceneData.particleSystemRenderers[id] = component;
//}

// Rigidbodoy
template <>
void Scene::registerComponent(entityID id, Rigidbody r) {
	const auto& t = sceneData.entities[id]->transform;
	if (r._bodyGenerated() == false) {
		r._generateBody(&bworld, t.position, t.rotation);
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
	s._generateBody(&bworld, t.position, t.rotation);
	sceneData.staticbodies[id] = s;
};

