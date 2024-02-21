#include "stdafx.h"

#include <vector>

#include <nlohmann/json.hpp>
#include <robin_hood.h>
#include <assetPack/SceneData_generated.h>

#include "ECS.h"
#include "Physics.h"

#include "SceneData.h"

using namespace nlohmann;
using namespace std;

nlohmann::json SceneData::serializeJson(bool ignorePersistence) const {

	nlohmann::json j;

	for (auto& [id, e] : entities) {
		if (e.persistent || ignorePersistence) {
			j["entities"].push_back(e.serializeJson());
		}
	}

	for (auto& [id, b] : behaviours)
	{
		if (entities.at(id).persistent || ignorePersistence) {

			nlohmann::json j2;
			{
				j2["entityID"] = id;
				j2["hash"] = b->Hash;

				auto props = b->getProperties();

				for (auto& prop : props)
				{
					j2["properties"].push_back(prop.serializeJson());
				}
			}

			j["behaviours"].push_back(j2);
		}
	}

	for (auto& s : spriteRenderers) {
		if (entities.at(s.first).persistent || ignorePersistence) {
			j["spriteRenderers"].push_back(s.second.serializeJson(s.first));
		}
	}
	for (auto& c : colorRenderers) {
		if (entities.at(c.first).persistent || ignorePersistence) {
			j["colorRenderers"].push_back(c.second.serializeJson(c.first));
		}
	}
	for (auto& c : textRenderers) {
		if (entities.at(c.first).persistent || ignorePersistence) {
			j["textRenderers"].push_back(c.second.serializeJson(c.first));
		}
	}
	for (auto& c : particleSystemRenderers) {
		if (entities.at(c.first).persistent || ignorePersistence) {
			j["particleSystemRenderers"].push_back(c.second.serializeJson(c.first));
		}
	}
	for (auto& r : rigidbodies) {
		if (entities.at(r.first).persistent || ignorePersistence) {
			j["rigidbodies"].push_back(r.second.serializeJson(r.first));
		}
	}
	for (auto& s : staticbodies) {
		if (entities.at(s.first).persistent || ignorePersistence) {
			j["staticbodies"].push_back(s.second.serializeJson(s.first));
		}
	}

	return j;
}

void SceneData::deserializeJson(nlohmann::json& j, SceneData* sceneData) {

	for (auto& e : j["entities"]) {
		entityID id = Entity::PeakID(e);
		// must have been persitent to have been serialized in the first place
		Entity* entity = sceneData->EmplaceEntity(id);
		Entity::deserializeJson(e, entity);
		entity->persistent = true;
	}

	for (auto& e : j["behaviours"]) {
		entityID entID = e["entityID"];
		behavioiurHash hash = e["hash"];

		auto [iter, inserted] = sceneData->behaviours.emplace(entID, BehaviorMap.at(hash).second(hash, &sceneData->entities.at(entID)));

		std::vector<SerializableProperty> props = iter->second->getProperties();
		
		for (auto& jprop : e["properties"])
		{
			std::string pname = jprop["name"];
			for (auto& prop : props)
			{
				if (pname == prop.name) {
					assert(prop.type == static_cast<SerializableProperty::Type>(jprop["type"]));
					prop.assignValue(jprop);
					break;
				}
			}
		}
	}


	for (auto& e : j["colorRenderers"]) {
		entityID entID = e["entityID"];
		ColorRenderer r = ColorRenderer::deserializeJson(e);
		sceneData->colorRenderers.insert({ entID, r });
	}
	for (auto& e : j["spriteRenderers"]) {
		entityID entID = e["entityID"];
		SpriteRenderer r = SpriteRenderer::deserializeJson(e);
		sceneData->spriteRenderers.insert({ entID, r });
	}
	for (auto& e : j["textRenderers"]) {
		entityID entID = e["entityID"];
		TextRenderer r = TextRenderer::deserializeJson(e);
		sceneData->textRenderers.insert({ entID, r });
	}
	for (auto& e : j["particleSystemRenderers"]) {
		entityID entID = e["entityID"];
		sceneData->particleSystemRenderers.emplace(entID, e);
	}
	for (auto& e : j["rigidbodies"]) {
		entityID entID = e["entityID"];
		sceneData->rigidbodies.emplace(entID, e);
		/*Rigidbody r = Rigidbody::deserializeJson(e);
		sceneData->rigidbodies.insert({ entID, r });*/
	}
	for (auto& e : j["staticbodies"]) {
		entityID entID = e["entityID"];
		sceneData->staticbodies.emplace(entID, e);
	//	Staticbody r = Staticbody::deserializeJson(e);
	//	sceneData->staticbodies.insert({ entID, r });
	}
}

void SceneData::deserializeFlatbuffers(const AssetPack::SceneData* s, SceneData* sceneData) {

	for (size_t i = 0; i < s->entities()->size(); i++) {

		const auto& fb = s->entities()->Get(i);
		entityID id = Entity::PeakID(fb);
		// must have been persitent to have been serialized in the first place
		Entity* entity = sceneData->EmplaceEntity(id);
		Entity::deserializeFlatbuffers(fb, entity);
		entity->persistent = true;
	}
	for (size_t i = 0; i < s->colorRenderers()->size(); i++) {
		ColorRenderer r = ColorRenderer::deserializeFlatbuffers(s->colorRenderers()->Get(i));
		entityID entID = s->colorRenderers()->Get(i)->entityID();
		sceneData->colorRenderers.insert({ entID, r });
	}
	for (size_t i = 0; i < s->spriteRenderers()->size(); i++) {
		SpriteRenderer r = SpriteRenderer::deserializeFlatbuffers(s->spriteRenderers()->Get(i));
		entityID entID = s->spriteRenderers()->Get(i)->entityID();
		sceneData->spriteRenderers.insert({ entID, r });
	}
	for (size_t i = 0; i < s->textRenderers()->size(); i++) {
		TextRenderer r = TextRenderer::deserializeFlatbuffers(s->textRenderers()->Get(i));
		entityID entID = s->textRenderers()->Get(i)->entityID();
		sceneData->textRenderers.insert({ entID, r });
	}
	for (size_t i = 0; i < s->particleSystemRenderers()->size(); i++) {
		entityID entID = s->particleSystemRenderers()->Get(i)->entityID();
		sceneData->particleSystemRenderers.emplace(entID, s->particleSystemRenderers()->Get(i));
	}
	assert(false);
	//for (size_t i = 0; i < s->rigidbodies()->size(); i++) {
	//	Rigidbody r = Rigidbody::deserializeFlatbuffers(s->rigidbodies()->Get(i));
	//	entityID entID = s->rigidbodies()->Get(i)->entityID();
	//	sceneData->rigidbodies.insert({ entID, r });
	//}
	//for (size_t i = 0; i < s->staticbodies()->size(); i++) {
	//	Staticbody r = Staticbody::deserializeFlatbuffers(s->staticbodies()->Get(i));
	//	entityID entID = s->staticbodies()->Get(i)->entityID();
	//	sceneData->staticbodies.insert({ entID, r });
	//}
}

Entity* SceneData::EmplaceEntity(entityID ID) {

	if (EntityGenerator.Contains(ID) == false) {
		// new entity
		EntityGenerator.Input(ID);
		auto [iterator, inserted] = entities.emplace(std::piecewise_construct, std::forward_as_tuple(ID), std::tuple<>());
		iterator->second.ID = ID;
		return &iterator->second;
	}
	else {
		// overwrite entity
		entities.insert_or_assign(ID, Entity());
		Entity* e = &entities.at(ID);
		e->ID = ID;
		return e;
	}
}

std::vector<spriteID> SceneData::getUsedSprites() const {
	std::vector<spriteID> ids;
	for (auto& i : spriteRenderers) {
		ids.push_back(i.second.sprite);
	}
	return ids;
};

std::vector<fontID> SceneData::getUsedFonts() const {
	std::vector<fontID> ids;
	for (auto& i : textRenderers) {
		ids.push_back(i.second.font);
	}
	return ids;
};