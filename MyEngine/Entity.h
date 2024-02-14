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

	nlohmann::json serializeJson();
	static Transform deserializeJson(const nlohmann::json& j);

	static Transform deserializeFlatbuffers(const AssetPack::Transform* t) {
		return Transform(
			fromAP(t->position()),
			fromAP(t->scale()),
			t->rotation());
	}
};

#include "ColorRenderer.h"
#include "SpriteRenderer.h"
#include "Physics.h"

class ComponentAccessor {
public:

	std::function<ColorRenderer* (entityID)> _getColorRenderer;
	std::function<SpriteRenderer* (entityID)> _getSpriteRenderer;
	std::function<Staticbody* (entityID)> _getStaticbody;
	std::function<Rigidbody* (entityID)> _getRigidbody;

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

	bool HasParent() const {
		return parent != nullptr;
	};
	bool HasChildren() const {
		return children.size() > 0;
	};

	std::set<Entity*> children;
	Entity* parent = nullptr;

	virtual void Start() {};
	virtual void Update() {};

	void _runStartUpdate() {
		if (startRan == false) {
			startRan = true;
			Start();
		}
		Update();
	}

	Transform GetGlobalTransform() const;

	glm::mat4 GetLocalToGlobalMatrix() const; // TODO: not actually a transform matrix. just get parent matrix and apply to current transofm and extract components if global position needed
	glm::mat4 GetGlobalToLocalMatrix() const;

	nlohmann::json serializeJson();
	static std::shared_ptr<Entity> deserializeJson(const nlohmann::json& j);

	static std::shared_ptr<Entity> deserializeFlatbuffers(const AssetPack::Entity* e);

	static std::shared_ptr<Input> input;
	static float DeltaTime;

protected:


	template <typename T>
	T* getComponent();

private:
	bool startRan = false;
	std::shared_ptr<ComponentAccessor> accessor = nullptr;

	void localTransformRecursive(glm::mat4* m) const;
	void globalTransformRecursive(glm::mat4* m) const;
};
