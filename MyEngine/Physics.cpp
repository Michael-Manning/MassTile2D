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


Rigidbody::Rigidbody(const nlohmann::json& j) : collider(j) {

	desc.linearDamping = j["linearDamping"];
	desc.angularDamping = j["angularDamping"];
	desc.fixedRotation = j["fixedRotation"];
	desc.bullet = j["bullet"];
	desc.gravityScale = j["gravityScale"];

	desc.friction = j["friction"];
	desc.density = j["density"];
	desc.restitution = j["restitution"];
}

Staticbody::Staticbody(const nlohmann::json& j) : collider(j) {

}