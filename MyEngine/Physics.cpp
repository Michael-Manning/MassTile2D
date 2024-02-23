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


Rigidbody::Rigidbody(const nlohmann::json& j) : collider(j["collider"]) {

	desc.linearDamping = j["linearDamping"];
	desc.angularDamping = j["angularDamping"];
	desc.fixedRotation = j["fixedRotation"];
	desc.bullet = j["bullet"];
	desc.gravityScale = j["gravityScale"];

	desc.friction = j["friction"];
	desc.density = j["density"];
	desc.restitution = j["restitution"];
}

Rigidbody::Rigidbody(const AssetPack::Rigidbody* b) : collider(b->collider()) {

	desc.linearDamping = b->linearDamping();
	desc.angularDamping = b->angularDamping();
	desc.fixedRotation = b->fixedRotation();
	desc.bullet = b->bullet();
	desc.gravityScale = b->gravityScale();

	desc.friction = b->friction();
	desc.density = b->density();
	desc.restitution = b->restitution();
}

Staticbody::Staticbody(const nlohmann::json& j) : collider(j["collider"]) {

}

Staticbody::Staticbody(const AssetPack::Staticbody* b) : collider(b->collider()){

}