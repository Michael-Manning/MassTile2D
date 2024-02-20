#pragma once

#include <unordered_map>
#include <string>
#include <set>

#include <robin_hood.h>

#include "ECS.h"
#include "Physics.h"
#include "IDGenerator.h"
#include "SceneData.h"
#include "Behaviour.h"
#include "BehaviorRegistry.h"

#include <assetPack/Scene_generated.h>

const auto Scene_extension = ".scene";

class Scene {
public:

	std::string name;

	SceneData sceneData;

	Scene();

	~Scene() {
		cleanup();
	}


	void serializeJson(std::string filename);
	static std::shared_ptr<Scene> deserializeJson(std::string filename);
	static std::shared_ptr<Scene> deserializeFlatbuffers(const AssetPack::Scene* scene);
	static std::string peakJsonName(std::string filename) {
		checkAppend(filename, Scene_extension);
		std::ifstream input(filename);
		nlohmann::json j;
		input >> j;
		return static_cast<std::string>(j["name"]);
	}

	void DeleteEntity(entityID id, bool deleteChildren);

	void ClearScene();

	Entity* GetEntity(entityID ID) {
		return &sceneData.entities.at(ID);
	};

	Entity* CreateEntity(Transform transform = {}, std::string name = "", bool persistent = false);

	Behaviour* AddBehaviour(Entity* entity, std::string behaviourName);
	Behaviour* AddBehaviour(Entity* entity, behavioiurHash hash);
	//Behaviour* AddBehaviour(entityID, std::string behaviourName);

	void SetEntityAsChild(Entity* parent, Entity* child);
	void SetEntityAsChild(entityID parent, entityID child) {
		SetEntityAsChild(GetEntity(parent), GetEntity(child));
	}


	Entity* DuplicateEntity(Entity* original);


	Entity* Instantiate(Prefab& prefab, std::string name = "prefab", glm::vec2 position = glm::vec2(0.0f), float rotation = 0.0f);

	double physicsTimer = 0.0;
	bool paused = true;

	template <typename T>
	void registerComponent(entityID id, T component);

	template <typename T>
	inline void registerComponent(std::shared_ptr<Entity> e, T component) {
		registerComponent(e->ID, component);
	};

	template <typename First, typename... Rest>
	void registerComponents(entityID id, First firstComponent, Rest... restComponents)
	{
		registerComponent(id, firstComponent);  // Register the first component.

		if constexpr (sizeof...(restComponents) > 0)
		{
			registerComponents(id, restComponents...);  // Recursively register the remaining components.
		}
	};

	template <>
	void registerComponent<ColorRenderer>(entityID id, ColorRenderer component);

	template <>
	void registerComponent<SpriteRenderer>(entityID id, SpriteRenderer component);

	template <>
	void registerComponent<TextRenderer>(entityID id, TextRenderer component);

	void registerComponent(entityID id, ParticleSystemRenderer::ParticleSystemSize systemSize);
	void registerComponent(entityID id, ParticleSystemRenderer::ParticleSystemSize systemSize, ParticleSystemPL::ParticleSystemConfiguration& configuration);

	template <>
	void registerComponent<Rigidbody>(entityID id, Rigidbody component);

	template <>
	void registerComponent<Staticbody>(entityID id, Staticbody component);

	// remove update responsibility from engine and make private again
	b2World bworld;

private:

	void deletechildRecurse(Entity* entity);

	void cleanup();

	// fills in entity cache pointers after deserializing a scene
	void LinkEntityRelationships();
	void LinkEntityRelationshipsRecurse(Entity* entity);

	std::unique_ptr<ComponentAccessor> componentAccessor;

	void linkRigidbodyB2D(entityID id, Rigidbody* r);
	void linkStaticbodyB2D(entityID id, Staticbody* r);

	void gatherChildIDsRecurse(std::vector<entityID>& IDs, Entity* entity);

	robin_hood::unordered_flat_map<std::string, std::pair<behavioiurHash, BehaviourFactoryFunc>> stringBehaviourMap;
	// 
	// copy of behaviour map, but directly hashable by name

	//robin_hood::unordered_flat_map<std::string, std::function<std::unique_ptr<Behaviour>(ComponentAccessor*, Entity*)>> stringBehaviourMap;

#ifdef USING_EDITOR
public:
	std::string GetNoneConflictingEntityName(Entity* entity, Entity* hierarchyLevel);
#endif

};