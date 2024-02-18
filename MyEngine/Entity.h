#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <set>
#include <optional>
#include <memory>
#include <typeinfo>
#include <robin_hood.h>

#include "typedefs.h"
//#include "ECS.h"
#include "Component.h"
#include "Utils.h"
#include "Input.h"
#include "serialization.h"

#include <assetPack/common_generated.h>

struct Transform {
	glm::vec2 position = glm::vec2(0.0f);
	glm::vec2 scale = glm::vec2(1.0f);
	float rotation = 0.0f;

	Transform(glm::vec2 position = glm::vec2(0.0f), glm::vec2 scale = glm::vec2(1.0f), float rotation = 0.0f) {
		this->position = position;
		this->scale = scale;
		this->rotation = rotation;
	}

	nlohmann::json serializeJson() const;
	static Transform deserializeJson(const nlohmann::json& j);

	static Transform deserializeFlatbuffers(const AssetPack::Transform* t) {
		return Transform(
			fromAP(t->position()),
			fromAP(t->scale()),
			t->rotation());
	}
};


class Entity {

public:

	Entity(std::string name = "", bool persistent = false) {
		this->name = name;
		this->persistent = persistent;
	};


	int ChildCount() const {
		return children.size();
	};
	inline void AddChild(Entity* entity) {
		children.insert(entity->ID);
		children_cachePtr.insert(entity);

	};
	inline void RemoveChild(Entity* entity) {
		assert(children_cachePtr.contains(entity));
		children_cachePtr.erase(entity);
		children.erase(entity->ID);
	};
	inline void SetParent(Entity* entity) {
		parent = entity->ID;
		parent_cachePtr = entity;
	}
	inline void ClearParent() {
		parent = NULL_Entity;
		parent_cachePtr = nullptr;
	};

	entityID GetParent() {
		return parent;
	}

	entityID* _GetParent() {
		return &parent;
	}
	robin_hood::unordered_flat_set<entityID>* _GetChildSet() {
		return &children;
	}
	robin_hood::unordered_flat_set<Entity*>* _GetChildCache_ptr() {
		return &children_cachePtr;
	};
	Entity** _GetParentCache_ptr() {
		return &parent_cachePtr;
	}

	// copy any memberwise values 
	void CloneInto(Entity* entity) {
		entity->name = name;
		entity->transform = transform;
	}

	entityID ID;

	Transform transform;

	std::string name;

	bool persistent = false;

	bool HasParent() const {
		return parent != NULL_Entity;
	};
	bool HasChildren() const {
		return children.size() > 0;
	};

	Transform GetGlobalTransform() const;

	glm::mat4 GetLocalToGlobalMatrix() const; // TODO: not actually a transform matrix. just get parent matrix and apply to current transofm and extract components if global position needed
	glm::mat4 GetGlobalToLocalMatrix() const;

	nlohmann::json serializeJson() const;
	static void deserializeJson(const nlohmann::json& j, Entity* entity);
	static void deserializeFlatbuffers(const AssetPack::Entity* packEntity, Entity* entity);
	static entityID PeakID(const nlohmann::json& j) {return j["id"].get<int>();}
	static entityID PeakID(const AssetPack::Entity* packEntity) {return packEntity->id();}

protected:


	template <typename T>
	T* getComponent();

private:

	// keeping child/parent cache in sync is none optional
	robin_hood::unordered_flat_set<entityID> children;
	entityID parent = NULL_Entity;
	robin_hood::unordered_flat_set<Entity*> children_cachePtr;
	Entity* parent_cachePtr = nullptr;

	void localTransformRecursive(glm::mat4* m) const;
	void globalTransformRecursive(glm::mat4* m) const;
};
