#pragma once

#include <unordered_map>
#include <string>
#include <set>
#include "ECS.h"
#include "Physics.h"
#include "IDGenerator.h"

#include <assetPack/Scene_generated.h>

const auto Scene_extension = ".scene";

struct SceneData {
	std::unordered_map<entityID, Entity> entities;

	// components
	std::unordered_map<entityID, SpriteRenderer> spriteRenderers;
	std::unordered_map<entityID, ColorRenderer> colorRenderers;
	std::unordered_map<entityID, Rigidbody> rigidbodies;
	std::unordered_map<entityID, Staticbody> staticbodies;
	std::unordered_map<entityID, TextRenderer> textRenderers;
	std::unordered_map<entityID, ParticleSystemRenderer> particleSystemRenderers;


	std::vector<spriteID> getUsedSprites() {
		std::vector<spriteID> ids;
		for (auto& i : spriteRenderers) {
			ids.push_back(i.second.sprite);
		}
		return ids;
	};
	std::vector<fontID> getUsedFonts() {
		std::vector<fontID> ids;
		for (auto& i : textRenderers) {
			ids.push_back(i.second.font);
		}
		return ids;
	};
};

class Scene {
public:

	std::string name;

	Scene() : bworld(gravity) {
		CreateComponentAccessor();
	};

	~Scene() {

		// must destroy all registrered physics objects and world.
		// either do it here, or have explicit unload function and then verify desctruction here
		assert(false);
	}

	SceneData sceneData;	

	void serializeJson(std::string filename);
	static std::shared_ptr<Scene> deserializeJson (std::string filename);
	static std::shared_ptr<Scene> deserializeFlatbuffers(const AssetPack::Scene * scene);
	static std::string peakJsonName(std::string filename) {
		checkAppend(filename, ".scene");
		std::ifstream input(filename);
		nlohmann::json j;
		input >> j;
		return static_cast<std::string>(j["name"]);
	}

	void UnregisterEntity(entityID id);

	//entityID RegisterEntity(std::shared_ptr<Entity> entity);

	Entity* CreateEntity(Transform transform = {}, std::string name = "", bool persistent = false);

	void OverwriteEntity(std::shared_ptr<Entity> entity, entityID ID);
	void RegisterAsChild(std::shared_ptr<Entity> parent, std::shared_ptr<Entity> child);

	// returns ID of new duplicate
	entityID DuplicateEntity(entityID original);

	Prefab CreatePrefab(std::shared_ptr<Entity> entity);

	Entity* Instantiate(Prefab prefab, std::string name = "prefab", glm::vec2 position = glm::vec2(0.0f), float rotation = 0.0f);

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
	
	void registerComponent_ParticleSystem(entityID id, ParticleSystemRenderer::ParticleSystemSize systemSize);

	template <>
	void registerComponent<Rigidbody>(entityID id, Rigidbody component);

	template <>
	void registerComponent<Staticbody>(entityID id, Staticbody component);

	// remove update responsibility from engine and make private again
	b2World bworld;

private:

	IDGenerator<entityID> EntityGenerator;

	std::shared_ptr<ComponentAccessor>	componentAccessor;

	void CreateComponentAccessor();
};