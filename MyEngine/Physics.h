#pragma once
#include <memory>
#include <cassert>

#include<glm/glm.hpp>

#pragma warning(push, 0)
#include <box2d/box2d.h>
#pragma warning(pop)

#include "typedefs.h"
#include "serialization.h"
#include "Component.h"

#include <assetPack/common_generated.h>

const b2Vec2 gravity = b2Vec2(0.0f, -10.0f);

// Convert glm::vec2 to b2Vec2
inline b2Vec2 gtb(const glm::vec2& vec) {
	return b2Vec2(vec.x, vec.y);
}

// Convert b2Vec2 to glm::vec2
constexpr glm::vec2 btg(const b2Vec2& vec) {
	return glm::vec2(vec.x, vec.y);
}

class Rigidbody;
class Staticbody;


class Collider {
public:

	enum class Type {
		Box,
		Circle
	};

	Type GetType() { return type; }

	Collider(glm::vec2 scale) {
		this->scale = scale;
		type = Type::Box;
		boxShape.SetAsBox(scale.x / 2.0f, scale.y / 2.0f);
	}
	Collider(float radius) {
		this->scale.x = radius;
		type = Type::Circle;
		circleShape.m_radius = radius / 2.0f;
	}

	glm::vec2 GetScale() { return scale; }
	void setScale(glm::vec2 scale) {
		assert(type == Type::Box);
		this->scale = scale;
		boxShape.SetAsBox(scale.x / 2.0f, scale.y / 2.0f);
	}

	float GetRadius() { return scale.x; }
	void setRadius(float radius) {
		assert(type == Type::Circle);
		this->scale.x = radius;
		circleShape.m_radius = radius / 2.0f;
	}

	nlohmann::json serializeJson() const {
		nlohmann::json j;
		j["type"] = type;
		j["scale"] = toJson(scale);
		return j;
	};

	Collider(const nlohmann::json& j) {
		type = static_cast<Type>(j["type"]);
		scale = fromJson<glm::vec2>(j["scale"]);
		if (type == Type::Box) {
			boxShape.SetAsBox(scale.x / 2.0f, scale.y / 2.0f);
		}
		else if (type == Type::Circle) {
			circleShape.m_radius = scale.x / 2.0f;
		}
		else {
			assert(false);
		}
	}

	Collider(const AssetPack::Collider& c) {
		type = static_cast<Type>(c.type());
		scale = fromAP(c.scale());
		if (type == Type::Box) {
			boxShape.SetAsBox(scale.x / 2.0f, scale.y / 2.0f);
		}
		else if (type == Type::Circle) {
			circleShape.m_radius = scale.x / 2.0f;
		}
		else {
			assert(false);
		}
	}

	~Collider() {
	}
	Collider(const Collider& other) = default;
	Collider& operator=(const Collider& other) = default;

private:

	glm::vec2 scale;

	Collider() = delete;

	friend Rigidbody;
	friend Staticbody;

	b2Shape* getShape() {
		if (type == Type::Box)
			return &boxShape;
		else
			return &circleShape;
	}

	Type type;
	b2PolygonShape boxShape;
	b2CircleShape circleShape;
};


class Staticbody : Component {
public:

	Staticbody(Collider collider) : collider(collider) {};

	// whether internal box2D data is tied to world in a scene
	bool IsLinked() {
		return linked;
	}

	void _LinkWorld(b2World* world, glm::vec2 position, float angle) {
		this->world = world;

		b2BodyDef bodyDef;
		bodyDef.position = gtb(position);
		bodyDef.angle = angle;

		body = world->CreateBody(&bodyDef);
		body->CreateFixture(collider.getShape(), 0.0f);

		linked = true;
	}


	~Staticbody() {
		//assert(world != nullptr);
		Destroy();
	}

	Staticbody(Staticbody&& other) noexcept = default; // Move constructor

	void Unlink() {
		assert(linked == true);

		Destroy();
		world = nullptr;

		linked = false;
	}

	void Destroy() {
		if (_bodyGenerated()) {
			assert(world != nullptr);
			body->DestroyFixture(fixture);
			world->DestroyBody(body);
			body = nullptr;
			fixture = nullptr;
		}
	};

	void updateCollider() {
		body->DestroyFixture(fixture);
		fixture = body->CreateFixture(collider.getShape(), 0.0f);
	};

	// Clone()
	Staticbody(const Staticbody& original, b2World* world) : collider(original.collider) {
		if (world != nullptr) {
			_LinkWorld(world, btg(original.body->GetPosition()), original.body->GetAngle());
		}
	}


	void SetTransform(glm::vec2 position, float rotation) {
		body->SetTransform(gtb(position), rotation);
		body->SetAwake(true);
	};

	void SetPosition(glm::vec2& position) {
		body->SetTransform(gtb(position), body->GetAngle());
		body->SetAwake(true);
	};
	void SetRotation(float rotation) {
		body->SetTransform(body->GetPosition(), rotation);
		body->SetAwake(true);
	};



	nlohmann::json serializeJson(entityID entId) const override;
	Staticbody(const nlohmann::json& j);
	Staticbody (const AssetPack::Staticbody* b);


	Collider collider;

private:

	bool linked = false;

	bool _bodyGenerated() const {
		return body != nullptr;
	};

	Staticbody(const Staticbody& other) = default;
	Staticbody& operator=(const Staticbody& other) = default;

	b2World* world = nullptr;

	b2Body* body = nullptr;
	b2Fixture* fixture = nullptr;
};


class Rigidbody : Component {

public:

	// NOTE: must delete collider default contructor if allowing rigidbody default constructor
	//Rigidbody() {
	//}

	~Rigidbody() {
		//assert(world != nullptr);
		Destroy();
	}

	Rigidbody(Collider collider) : collider(collider) {}

	Rigidbody(Rigidbody&& other) noexcept = default; // Move constructor

	// whether internal box2D data is tied to world in a scene
	bool IsLinked() {
		return linked;
	}

	// called upon registration
	void _LinkWorld(b2World* world, glm::vec2 position, float angle) {
		this->world = world;

		b2FixtureDef fixtureDef;
		fixtureDef.shape = collider.getShape();
		fixtureDef.density = desc.density;
		fixtureDef.friction = desc.friction;
		fixtureDef.restitution = desc.restitution;

		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(position.x, position.y);
		bodyDef.angle = angle;
		bodyDef.linearDamping = desc.linearDamping;
		bodyDef.angularDamping = desc.angularDamping;
		bodyDef.fixedRotation = desc.fixedRotation;
		bodyDef.bullet = desc.bullet;
		bodyDef.gravityScale = desc.gravityScale;

		body = world->CreateBody(&bodyDef);
		fixture = body->CreateFixture(&fixtureDef);

		linked = true;
	};

	// could running copy of body/fixture def as params are updated for faster copy?
	// Clone()
	Rigidbody(const Rigidbody& original, b2World* world) :collider(original.collider) {

		desc.linearDamping = original.desc.linearDamping;
		desc.angularDamping = original.desc.angularDamping;
		desc.fixedRotation = original.desc.fixedRotation;
		desc.bullet = original.desc.bullet;
		desc.gravityScale = original.desc.gravityScale;

		desc.friction = original.desc.friction;
		desc.density = original.desc.density;
		desc.restitution = original.desc.restitution;

		if (world != nullptr) {
			if(original.body != nullptr)
			_LinkWorld(world, btg(original.body->GetPosition()), original.body->GetAngle());	
			else
			_LinkWorld(world, glm::vec2(0.0f), 0.0f);	
		}
	}

	void Unlink() {

		assert(linked == true);

		Destroy();
		world = nullptr;

		linked = false;
	}

	void Destroy() {
		if (_bodyGenerated()) {
			assert(world != nullptr);
			body->DestroyFixture(fixture);
			world->DestroyBody(body);
			body = nullptr;
		}
	};

	void updateCollider() {

		b2FixtureDef fixtureDef;
		fixtureDef.shape = collider.getShape();
		fixtureDef.density = fixture->GetDensity();
		fixtureDef.friction = fixture->GetFriction();
		fixtureDef.restitution = fixture->GetRestitution();

		body->DestroyFixture(fixture);
		fixture = body->CreateFixture(&fixtureDef);
	};

	void SetTransform(glm::vec2 position, float rotation) {
		body->SetTransform(gtb(position), rotation);
		body->SetAwake(true);
	};

	void SetPosition(glm::vec2& position) {
		body->SetTransform(gtb(position), body->GetAngle());
		body->SetAwake(true);
	};
	glm::vec2 _getPosition() const {
		return btg(body->GetPosition());
	}

	void SetRotation(float rotation) {
		body->SetTransform(body->GetPosition(), rotation);
		body->SetAwake(true);
	};
	float _getRotation() {
		return body->GetAngle();
	}

	void SetAngularVelocity(float velocity) {
		body->SetAngularVelocity(velocity);
	};
	float GetAngularVelocity() const {
		return body->GetAngularVelocity();
	};

	void SetLinearDamping(float damping) {
		body->SetLinearDamping(damping);
		desc.linearDamping = damping;
	};
	float GetLinearDamping() const {
		return body->GetLinearDamping();
	};

	void SetAngularDamping(float damping) {
		body->SetAngularDamping(damping);
		desc.angularDamping = damping;
	};
	float GetAngularDamping() const {
		return body->GetAngularDamping();
	};

	void SetFixedRotation(bool fixed) {
		body->SetFixedRotation(fixed);
		desc.fixedRotation = fixed;
	};
	bool GetFixedRotation() const {
		return body->IsFixedRotation();
	};

	void SetBullet(bool bullet) {
		body->SetBullet(bullet);
		desc.bullet = bullet;
	};
	bool GetBullet() const {
		return body->IsBullet();
	};

	void SetGravityScale(float scale) {
		body->SetGravityScale(scale);
		desc.gravityScale = scale;
	};
	float GetGravityScale() const {
		return body->GetGravityScale();
	};

	void SetFriction(float friction) {
		fixture->SetFriction(friction);
		desc.friction = friction;
	};
	float GetFriction() const {
		return fixture->GetFriction();
	};

	void SetDensity(float density) {
		fixture->SetDensity(density);
		desc.density = density;
	};
	float GetDensity() const {
		return fixture->GetDensity();
	};

	void SetRestitution(float restitution) {
		fixture->SetRestitution(restitution);
		desc.restitution = restitution;
	};
	float GetRestitution() const {
		return fixture->GetRestitution();
	};

	//////////////////////////////


	void SetLinearVelocity(glm::vec2 velocity) {
		body->SetLinearVelocity(gtb(velocity));
	};
	glm::vec2 GetLinearVelocity() {
		return btg(body->GetLinearVelocity());
	};
	void AddForce(glm::vec2 force) {
		body->ApplyForceToCenter(gtb(force), true);
	}



	nlohmann::json serializeJson(entityID entId) const override;
	Rigidbody(const nlohmann::json& j);
	Rigidbody(const AssetPack::Rigidbody* b);

	Collider collider;

private:

	bool linked = false;

	bool _bodyGenerated() const {
		return body != nullptr;
	};

	Rigidbody(const Rigidbody& other) = default;
	Rigidbody& operator=(const Rigidbody& other) = default;

	// state of body and fixture must be stored outside of world context as the rigidbody
	// can be created before it is registered to a scene/world
	struct RigidbodyDescription {
		float linearDamping = 0.1f;
		float angularDamping = 0.1f;
		bool fixedRotation = false;
		bool bullet = false;
		float gravityScale = 1.0f;
		float friction = 0.3f;
		float density = 1.0f;
		float restitution = 0.5f;
	};
	RigidbodyDescription desc; // currently only used for construction at registration

	b2Body* body = nullptr;
	b2Fixture* fixture = nullptr;

	b2World* world;
};

