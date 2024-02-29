#include "stdafx.h"

#include <vector>

#include <nlohmann/json.hpp>
#include <robin_hood.h>
#include <assetPack/SceneData_generated.h>
#include <assetPack/common_generated.h>

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
		classHash hash = e["hash"];

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
		sceneData->spriteRenderers.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(entID),
			std::forward_as_tuple(r, &sceneData->entities.at(entID)));
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
	}
	for (auto& e : j["staticbodies"]) {
		entityID entID = e["entityID"];
		sceneData->staticbodies.emplace(entID, e);
	}
}

SceneData::SceneData(const AssetPack::SceneData* sceneData) {

	for (size_t i = 0; i < sceneData->entities()->size(); i++) {

		auto pack = sceneData->entities()->Get(i);
		entityID ID = pack->id();
		assert(EntityGenerator.Contains(ID) == false);
		EntityGenerator.Input(ID);
		auto [iterator, inserted] = entities.emplace(ID, pack);
		iterator->second.persistent = true;
	}


	if (sceneData->behaviours() != nullptr) {
		for (size_t i = 0; i < sceneData->behaviours()->size(); i++)
		{
			auto pack = sceneData->behaviours()->Get(i);
			entityID entID = pack->entityID();
			classHash hash = pack->hash();

			auto [iter, inserted] = behaviours.emplace(entID, BehaviorMap.at(hash).second(hash, &entities.at(entID)));

			std::vector<SerializableProperty> props = iter->second->getProperties();

			if (pack->properties() != nullptr) {
				for (size_t j = 0; j < pack->properties()->size(); j++)
				{
					auto packProp = pack->properties()->Get(j);
					std::string pname = packProp->name()->str();

					for (auto& prop : props)
					{
						if (pname == prop.name) {
							assert(prop.type == static_cast<SerializableProperty::Type>(packProp->type()));
							prop.assignValue(packProp);
							break;
						}
					}

				}
			}
		}
	}

	if (sceneData->colorRenderers() != nullptr) {
		for (size_t i = 0; i < sceneData->colorRenderers()->size(); i++) {
			ColorRenderer r = ColorRenderer::deserializeFlatbuffers(sceneData->colorRenderers()->Get(i));
			entityID entID = sceneData->colorRenderers()->Get(i)->entityID();
			colorRenderers.insert({ entID, r });
		}
	}
	if (sceneData->spriteRenderers() != nullptr) {
		for (size_t i = 0; i < sceneData->spriteRenderers()->size(); i++) {
			auto pack = sceneData->spriteRenderers()->Get(i);
			entityID entID = sceneData->spriteRenderers()->Get(i)->entityID();
			spriteRenderers.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(entID),
				std::forward_as_tuple(pack, &entities.at(entID)));
		}
	}
	if (sceneData->textRenderers() != nullptr) {
		for (size_t i = 0; i < sceneData->textRenderers()->size(); i++) {
			TextRenderer r = TextRenderer::deserializeFlatbuffers(sceneData->textRenderers()->Get(i));
			entityID entID = sceneData->textRenderers()->Get(i)->entityID();
			textRenderers.insert({ entID, r });
		}
	}
	if (sceneData->particleSystemRenderers() != nullptr) {
		for (size_t i = 0; i < sceneData->particleSystemRenderers()->size(); i++) {
			entityID entID = sceneData->particleSystemRenderers()->Get(i)->entityID();
			particleSystemRenderers.emplace(entID, sceneData->particleSystemRenderers()->Get(i));
		}
	}
	if (sceneData->rigidbodies() != nullptr) {
		for (size_t i = 0; i < sceneData->rigidbodies()->size(); i++) {
			auto pack = sceneData->rigidbodies()->Get(i);
			rigidbodies.emplace(pack->entityID(), pack);
		}
	}
	if (sceneData->staticbodies() != nullptr) {
		for (size_t i = 0; i < sceneData->staticbodies()->size(); i++) {
			auto pack = sceneData->staticbodies()->Get(i);
			staticbodies.emplace(pack->entityID(), pack);
		}
	}
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