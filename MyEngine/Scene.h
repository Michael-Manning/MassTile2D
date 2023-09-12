#pragma once

#include <unordered_map>
#include <string>
#include <set>
#include "ECS.h"
#include "Physics.h"
#include "IDGenerator.h"
#include "AssetManager.h"


struct SceneData {
	std::unordered_map<entityID, std::shared_ptr<Entity>> entities;

	//// assets
	//std::unordered_map<spriteID, std::shared_ptr<Sprite>> sprites;

	// components
	std::unordered_map<entityID, SpriteRenderer> spriteRenderers;
	std::unordered_map<entityID, ColorRenderer> colorRenderers;
	std::unordered_map<entityID, Rigidbody> rigidbodies;
	std::unordered_map<entityID, Staticbody> staticbodies;
	std::unordered_map<entityID, TextRenderer> textRenderers;


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

	Scene(std::shared_ptr<AssetManager> _assetManager) : assetManager(_assetManager) {
		CreateComponentAccessor();
	};


	SceneData sceneData;	
	

	void CreateDefaultSprite(int w, int h, std::vector<uint8_t>& data);

	IDGenerator<entityID> EntityGenerator;


	//type appended
	void SaveScene(std::string filename);
	void LoadScene(std::string filename, std::shared_ptr<b2World> world);

	void UnregisterEntity(entityID id);

	void RegisterEntity(std::shared_ptr<Entity> entity);
	void OverwriteEntity(std::shared_ptr<Entity> entity, entityID ID);
	void RegisterAsChild(std::shared_ptr<Entity> parent, std::shared_ptr<Entity> child);

	entityID DuplicateEntity(std::shared_ptr<Entity> entity);

	Prefab CreatePrefab(std::shared_ptr<Entity> entity);

	std::shared_ptr<Entity> Instantiate(Prefab& prefab, std::string name = "prefab", glm::vec2 position = glm::vec2(0.0f), float rotation = 0.0f);

	void setB2World(std::shared_ptr<b2World> bworld) {
		this->bworld = bworld;
	}

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

	template <>
	void registerComponent<Rigidbody>(entityID id, Rigidbody component);

	template <>
	void registerComponent<Staticbody>(entityID id, Staticbody component);

private:

	std::shared_ptr<AssetManager> assetManager;
	std::shared_ptr<b2World> bworld;
	std::shared_ptr<ComponentAccessor>	componentAccessor;

	void CreateComponentAccessor();
};