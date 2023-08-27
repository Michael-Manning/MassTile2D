#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include <set>
#include <optional>
#include <memory>
#include <typeinfo>

#include "typedefs.h"
//#include "ECS.h"
#include "Component.h"
#include "Utils.h"
#include "Input.h"

struct Transform {
	glm::vec2 position = glm::vec2(0.0f);
	glm::vec2 scale = glm::vec2(1.0f);
	float rotation = 0.0f;

	Transform(glm::vec2 position = glm::vec2(0.0f), glm::vec2 scale = glm::vec2(1.0f), float rotation = 0.0f) {
		this->position = position;
		this->scale = scale;
		this->rotation = rotation;
	}

	nlohmann::json serializeJson();
	static Transform deserializeJson(const nlohmann::json& j);
};

#include "ColorRenderer.h"
#include "SpriteRenderer.h"
#include "Physics.h"

class ComponentAccessor {
public:

	std::function<ColorRenderer*(entityID)> _getColorRenderer;
	std::function<SpriteRenderer*(entityID)> _getSpriteRenderer;
	std::function<Staticbody*(entityID)> _getStaticbody;
	std::function<Rigidbody*(entityID)> _getRigidbody;

	//std::shared_ptr<Input> _input;

	ComponentAccessor(
		std::function<ColorRenderer* (entityID)> colorRendererFunc,
		std::function<SpriteRenderer* (entityID)> spriteRendererFunc,
		std::function<Staticbody* (entityID)> staticbodyFunc,
		std::function<Rigidbody* (entityID)> rigidbodyFunc
	//	std::shared_ptr<Input> input
		) :
		_getColorRenderer(colorRendererFunc),
		_getSpriteRenderer(spriteRendererFunc),
		_getStaticbody(staticbodyFunc),
		_getRigidbody(rigidbodyFunc)
	//	_input(input)
	{};
};


class Entity {

public:

	Entity() {
		this->startRan = false;
	};
	Entity(std::string name, bool persistent = false) {
		this->name = name;
		this->persistent = persistent;
		this->startRan = false;
	};

	void _setComponentAccessor(std::shared_ptr<ComponentAccessor> accessor) {
		this->accessor = accessor;
	};

	virtual std::string GetEditorName() { return ""; }
	virtual uint32_t getBehaviorHash() { 
		return 0;
	}

	entityID ID;

	Transform transform;

	std::string name;

	bool persistent = false;

	std::set<entityID> children;
	std::optional<entityID> parent;

	virtual void Start() {};
	virtual void Update() {};

	void _runStartUpdate() {
		if (startRan == false) {
			startRan = true;
			Start();
		}
		Update();
	}

	nlohmann::json serializeJson();
	static std::shared_ptr<Entity> deserializeJson(const nlohmann::json& j);

	//std::shared_ptr<Entity> Duplicate() {
	//	std::shared_ptr<Entity> e = std::make_shared<Entity>();
	//	e->name = name + std::string("_copy");
	//	e->persistent = false;
	//	e->_setComponentAccessor(accessor);
	//	e->transform = transform;
	//	e->startRan = false;
	//};

	static std::shared_ptr<Input> input;
	static float DeltaTime;

protected:


	template <typename T>
	T* getComponent();

	//template <>
	//ColorRenderer* getComponent<ColorRenderer*>(entityID id);
	//ColorRenderer* getComponent();

private:
	bool startRan = false;
	std::shared_ptr<ComponentAccessor> accessor = nullptr;
};
