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

//class Collider {
//public:
//
//	virtual nlohmann::json serializeJson() = 0;
//
//	virtual b2Shape* _getB2Shape() = 0;
//
//	virtual int _getType() = 0;
//
//	virtual std::uni<Collider> duplicate() = 0;
//};
//
//class BoxCollider : public Collider {
//
//public:
//
//	BoxCollider(glm::vec2 scale) {
//		this->scale = scale;
//	}
//
//	b2Shape* _getB2Shape() override {
//		dynamicBox.SetAsBox(scale.x / 2.0f, scale.y / 2.0f);
//		return &dynamicBox;
//	}
//
//	nlohmann::json serializeJson() override {
//		nlohmann::json j;
//		j["type"] = _getType();
//		j["scale"] = toJson(scale);
//		return j;
//	};
//
//	int _getType() override {
//		return 1;
//	}
//
//	std::shared_ptr<Collider> duplicate() override {
//		return std::make_shared<BoxCollider>(scale);
//	}
//
//	glm::vec2 scale;
//
//private:
//	b2PolygonShape dynamicBox;
//};
//
//class CircleCollider : public Collider {
//
//public:
//
//	CircleCollider(float radius) {
//		this->radius = radius;
//	}
//
//	b2Shape* _getB2Shape() {
//		dynamicCircle.m_radius = radius / 2.0f;
//		return &dynamicCircle;
//	}
//
//	nlohmann::json serializeJson() override {
//		nlohmann::json j;
//		j["type"] = _getType();
//		j["radius"] = radius;
//		return j;
//	};
//
//	std::shared_ptr<Collider> duplicate() override {
//		return std::make_shared<CircleCollider>(radius);
//	}
//
//	float radius;
//
//	int _getType() override {
//		return 2;
//	}
//
//private:
//	b2CircleShape dynamicCircle;
//};

class Collider {
public:

	enum class Type {
		Box,
		Circle
	};


	nlohmann::json serializeJson();

	b2Shape* _getB2Shape();

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

	void setRadius(float radius) {

	}

	b2Shape* _getB2Shape() override {
		return &shape;
	}

	nlohmann::json serializeJson() override {
		nlohmann::json j;
		j["type"] = type();
		j["scale"] = toJson(scale);
		return j;
	};

	std::unique_ptr<Collider> clone() override {
		return std::make_shared<Collider>(scale);
	}

	glm::vec2 scale;

private:
	Type type;
	union {
		b2PolygonShape boxShape;
		b2CircleShape circleShape;
	};
};


static std::shared_ptr<Collider> Collider_deserializeJson(nlohmann::json j) {
	int type = j["type"];
	if (type == 1) {
		glm::vec2 scale = fromJson<glm::vec2>(j["scale"]);
		auto b = std::make_shared<BoxCollider>(scale);
		return  b;
	}
	else {
		float radius = j["radius"];
		auto b = std::make_shared<CircleCollider>(radius);
		return  b;
	}
};


class Staticbody : Component {
public:

	Staticbody() {}
	Staticbody(std::shared_ptr<Collider> collider) {
		this->collider = collider;
	};

	void _generateBody(b2World* world, glm::vec2 position, float angle) {
		this->world = world;

		b2BodyDef bodyDef;
		bodyDef.position = gtb(position);
		bodyDef.angle = angle;

		body = world->CreateBody(&bodyDef);

		body->CreateFixture(collider->_getB2Shape(), 0.0f);
	}


	~Staticbody() {
		//assert(world != nullptr);
		Destroy();
	}

	void Unlink() {
		Destroy();
		world = nullptr;
	}

	void Destroy() {
		if (_bodyGenerated()) {
			assert(world != nullptr);
			world->DestroyBody(body);
		}
	};

	bool _bodyGenerated() {
		return body != nullptr;
	};

	void updateCollider() {
		body->DestroyFixture(fixture);
		fixture = body->CreateFixture(collider->_getB2Shape(), 0.0f);
	};

	Staticbody duplicate() {
		assert(false); // haven't figured out if this works with new scene/world registration logic
		Staticbody sb(collider->duplicate());
		sb._generateBody(world, btg(body->GetPosition()), body->GetAngle());
		return sb;
	};


	void SetTransform(glm::vec2 position, float rotation) {
		body->SetTransform(gtb(position), rotation);
	};

	void SetPosition(glm::vec2& position) {
		body->SetTransform(gtb(position), body->GetAngle());
	};
	void SetRotation(float rotation) {
		body->SetTransform(body->GetPosition(), rotation);
	};



	nlohmann::json serializeJson(entityID entId) const override;
	static Staticbody deserializeJson(const nlohmann::json& j);

	static Staticbody deserializeFlatbuffers(const AssetPack::Staticbody* b) {

		std::shared_ptr<Collider> collider;
		auto fbCollider = b->collider();
		if (fbCollider.type() == 1) {
			collider = std::make_shared<BoxCollider>(fromAP(fbCollider.scale()));
		}
		else {
			collider = std::make_shared<CircleCollider>(fbCollider.radius());
		}

		Staticbody body(collider);
		return body;
	}

	std::shared_ptr<Collider> collider;

private:
	b2World* world;

	b2Body* body = nullptr;
	b2Fixture* fixture = nullptr;
};


class Rigidbody : Component {

public:

	Rigidbody() {
		//assert(false);
	}

	~Rigidbody() {
		//assert(world != nullptr);
		Destroy();
	}

	// position is overwritten by registerComponent
	Rigidbody(std::shared_ptr<Collider>  collider);

	// called upon registration
	void _generateBody(b2World* world, glm::vec2 position, float angle) {
		this->world = world;

		b2FixtureDef fixtureDef;
		fixtureDef.shape = collider->_getB2Shape();
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
	};

	// full body initialization. Used when deserialization so all parameters can be set before body is created
	//void _generateBody(b2World* world, b2FixtureDef* fixtureDef, b2BodyDef* bodyDef) {
	//	this->world = world;

	//	b2BodyDef bdef;
	//	bdef.linearDamping = linearDamping();
	//	bdef.angularDamping = b->angularDamping();
	//	bdef.fixedRotation = b->fixedRotation();
	//	bdef.bullet = b->bullet();
	//	bdef.gravityScale = b->gravityScale();

	//	b2FixtureDef fdef;
	//	fdef.friction = b->friction();
	//	fdef.density = b->density();
	//	fdef.restitution = b->restitution();

	//	fixtureDef->shape = collider->_getB2Shape();

	//	bodyDef->type = b2_dynamicBody;

	//	body = world->CreateBody(bodyDef);
	//	fixture = body->CreateFixture(fixtureDef);
	//};

	//~Rigidbody(){
	void Unlink() {
		Destroy();
		world = nullptr;
	}

	void Destroy() {
		if (_bodyGenerated()) {
			assert(world != nullptr);
			world->DestroyBody(body);
		}
	};

	bool _bodyGenerated() {
		return body != nullptr;
	};

	void updateCollider() {

		b2FixtureDef fixtureDef;
		fixtureDef.shape = collider->_getB2Shape();
		fixtureDef.density = fixture->GetDensity();
		fixtureDef.friction = fixture->GetFriction();
		fixtureDef.restitution = fixture->GetRestitution();

		body->DestroyFixture(fixture);
		fixture = body->CreateFixture(&fixtureDef);
	};

	// could running copy of body/fixture def as params are updated for faster copy?
	Rigidbody duplicate();


	void SetTransform(glm::vec2 position, float rotation) {
		body->SetTransform(gtb(position), rotation);
		body->SetAwake(true);
	};

	void SetPosition(glm::vec2& position) {
		body->SetTransform(gtb(position), body->GetAngle());
		body->SetAwake(true);
	};
	glm::vec2 _getPosition() {
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
	float GetAngularVelocity() {
		return body->GetAngularVelocity();
	};

	void SetLinearDamping(float damping) {
		body->SetLinearDamping(damping);
	};
	float GetLinearDamping() const {
		return body->GetLinearDamping();
	};

	void SetAngularDamping(float damping) const {
		body->SetAngularDamping(damping);
	};
	float GetAngularDamping() const {
		return body->GetAngularDamping();
	};

	void SetFixedRotation(bool fixed) const {
		body->SetFixedRotation(fixed);
	};
	bool GetFixedRotation() const {
		return body->IsFixedRotation();
	};

	void SetBullet(bool bullet) const {
		body->SetBullet(bullet);
	};
	bool GetBullet() const {
		return body->IsBullet();
	};

	void SetGravityScale(float scale) const {
		body->SetGravityScale(scale);
	};
	float GetGravityScale() const {
		return body->GetGravityScale();
	};

	void SetFriction(float friction) const {
		fixture->SetFriction(friction);
	};
	float GetFriction() const {
		return fixture->GetFriction();
	};

	void SetDensity(float density) const {
		fixture->SetDensity(density);
	};
	float GetDensity() const {
		return fixture->GetDensity();
	};

	void SetRestitution(float restitution) const {
		fixture->SetRestitution(restitution);
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

	static Rigidbody deserializeFlatbuffers(const AssetPack::Rigidbody* b) {
		Rigidbody r;

		auto fbCollider = b->collider();
		if (fbCollider.type() == 1) {
			r.collider = std::make_shared<BoxCollider>(fromAP(fbCollider.scale()));
		}
		else {
			r.collider = std::make_shared<CircleCollider>(fbCollider.radius());
		}

		r.desc.linearDamping = b->linearDamping();
		r.desc.angularDamping = b->angularDamping();
		r.desc.fixedRotation = b->fixedRotation();
		r.desc.bullet = b->bullet();
		r.desc.gravityScale = b->gravityScale();

		r.desc.friction = b->friction();
		r.desc.density = b->density();
		r.desc.restitution = b->restitution();
		//b2BodyDef bdef;
		//bdef.linearDamping = b->linearDamping();
		//bdef.angularDamping = b->angularDamping();
		//bdef.fixedRotation = b->fixedRotation();
		//bdef.bullet = b->bullet();
		//bdef.gravityScale = b->gravityScale();

		//b2FixtureDef fdef;
		//fdef.friction = b->friction();
		//fdef.density = b->density();
		//fdef.restitution = b->restitution();

		//r._generateBody(world, &fdef, &bdef);

		return r;

	}

	std::shared_ptr<Collider> collider;

private:

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

