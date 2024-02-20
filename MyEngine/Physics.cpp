#include "stdafx.h"

#include <memory>
#include <cassert>

#include<glm/glm.hpp>

#pragma warning(push, 0)
#include <box2d/box2d.h>
#pragma warning(pop)

#include "typedefs.h"
#include "serialization.h"
#include "Component.h"

#include "Physics.h"

Rigidbody Rigidbody::duplicate() {

	assert(false); // haven't figured out if this works with new scene/world registration logic

	Rigidbody r;
	r.collider = collider->duplicate();

	r.desc = desc; // ??

	/*b2BodyDef bd;
	bd.position = gtb(_getPosition());
	bd.angle = _getRotation();
	bd.linearDamping = GetLinearDamping();
	bd.angularDamping = GetAngularDamping();
	bd.fixedRotation = GetFixedRotation();
	bd.bullet = GetBullet();
	bd.gravityScale = GetGravityScale();

	b2FixtureDef  fd;
	fd.density = GetDensity();
	fd.friction = GetFriction();
	fd.restitution = GetRestitution();

	r._generateBody(world, &fd, &bd);*/

	return r;
}

Rigidbody::Rigidbody(std::shared_ptr<Collider>  collider) {
	this->collider = collider;
};

Rigidbody::Rigidbody(const nlohmann::json& j) {
	this->collider = Collider_deserializeJson(j["collider"]);

	desc.linearDamping = j["linearDamping"];
	desc.angularDamping = j["angularDamping"];
	desc.fixedRotation = j["fixedRotation"];
	desc.bullet = j["bullet"];
	desc.gravityScale = j["gravityScale"];

	desc.friction = j["friction"];
	desc.density = j["density"];
	desc.restitution = j["restitution"];
}