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
#include "Utils.h"
#include "Input.h"
#include "serialization.h"

#include <nlohmann/json.hpp>
#include <assetPack/common_generated.h>

class Scene;
class Behaviour;

struct ComponentAccessor {

	Scene* scene;

	robin_hood::unordered_node_map<entityID, Entity>* entities;

	robin_hood::unordered_node_map<entityID, std::unique_ptr<Behaviour>>* behaviours;
	robin_hood::unordered_node_map<entityID, SpriteRenderer>* spriteRenderers;
	robin_hood::unordered_node_map<entityID, ColorRenderer>* colorRenderers;
	robin_hood::unordered_node_map<entityID, Rigidbody>* rigidbodies;
	robin_hood::unordered_node_map<entityID, Staticbody>* staticbodies;
	robin_hood::unordered_node_map<entityID, TextRenderer>* textRenderers;
	robin_hood::unordered_node_map<entityID, ParticleSystemRenderer>* particleSystemRenderers;

};

struct SerializableProperty {
	enum class Type {
		INT = 0,
		FLOAT,
		VEC2
	};

	Type type;
	std::string name;
	void* value = nullptr;

	nlohmann::json serializeJson() const;
	void assignValue(const nlohmann::json& j);
	void assignValue(const AssetPack::SerializableProperty* s);
};

//struct PropertyGroup {
//	std::vector<SerializableProperty> properties;
//
//	nlohmann::json serializeJson() const;
//};

#define PROPERTY_EXPORT(...) \
    std::vector<SerializableProperty> getProperties()  override { \
        return {__VA_ARGS__}; \
    }


#define MAKE_PROPERTY(variable) makeProperty(#variable, &variable)


class Behaviour {

public:

	Behaviour(behavioiurHash classHash, Entity* entityCache) : Hash(classHash){
		this->startRan = false;
		_setEntity(entityCache);
	};


	virtual std::unique_ptr<Behaviour> clone(Entity* entity, ComponentAccessor * accessor) const = 0;

	const behavioiurHash Hash;

	virtual void Start() = 0;
	virtual void Update() = 0;
	virtual void OnDestroy() {};

	void Destory();
	//	//accessor->scene->DeleteEntity(entity->ID, true);
	//}

	void _runStartUpdate() {
		if (startRan == false) {
			startRan = true;
			Start();
		}

		Update();
	};

	virtual std::vector<SerializableProperty> getProperties() { return {}; };

	Entity* GetEntity() {
		return entity;
	};
	entityID GetEntityID() {
		return entID;
	};

	static Input* input;
	static float deltaTime;

	void _SetComponentAccessor(ComponentAccessor* accessor) {
		this->accessor = accessor;
	}
	void _ReplaceEntityLink(Entity* entity) {
		this->entity = entity;
		this->entID = entity->ID;
	}

protected:

	Transform* transform;
	Entity* entity;
	entityID entID;

	void _setEntity(Entity* entity) {
		this->entity = entity;
		entID = entity->ID;
		transform = &entity->transform;
	}

	template <typename T>
	T* getComponent();

	SerializableProperty makeProperty(const std::string& name, int* value) {
		return { SerializableProperty::Type::INT, name, static_cast<void*>(value) }; 
	}
	SerializableProperty makeProperty(const std::string& name, float* value) {
		return { SerializableProperty::Type::FLOAT, name, static_cast<void*>(value) };
	}
	SerializableProperty makeProperty(const std::string& name, glm::vec2* value) {
		return { SerializableProperty::Type::VEC2, name, static_cast<void*>(value) };
	}

private:


	bool startRan = false;
	ComponentAccessor* accessor = nullptr;
};

#define BEHAVIOUR_CONSTUCTOR(c) c(behavioiurHash b, Entity* e) : Behaviour(b, e)

#define BEHAVIOUR_CLONE(c)     std::unique_ptr<Behaviour> clone(Entity* entity, ComponentAccessor * accessor) const override { \
auto copy = std::make_unique<c>(*this); \
if(entity != nullptr){ \
copy->_setEntity(entity); \
copy->_SetComponentAccessor(accessor); \
} \
return copy; \
};