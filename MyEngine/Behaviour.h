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


struct ComponentAccessor {
	robin_hood::unordered_node_map<entityID, Entity>* entities;

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
	static void deserialize(const nlohmann::json& j, SerializableProperty* sp);
	static void deserialize(const AssetPack::SerializableProperty* s, SerializableProperty* sp);
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

	Behaviour(behavioiurHash classHash, ComponentAccessor* accessor, Entity* entityCache) : Hash(classHash), entity(entityCache){
		this->startRan = false;
		this->transform = &entity->transform;
		this->accessor = accessor;

	};

	const behavioiurHash Hash;

	virtual void Start() = 0;
	virtual void Update() = 0;

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

	static Input* input;
	static float deltaTime;


protected:

	Transform* transform;
	Entity* entity;

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

#define BEHAVIOUR_CONSTUCTOR(c) c(behavioiurHash b, ComponentAccessor* a, Entity* e) : Behaviour(b, a, e) {}