#pragma once

#include <optional>
#include <string>
#include <fstream>
#include <memory>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "ECS.h"
#include "serialization.h"
#include <box2d/box2d.h>

const auto Prefab_extension = ".prefab";

class Prefab {
public:

	uint32_t behaviorHash;
	Transform transform;

	std::optional<ColorRenderer> colorRenderer;
	std::optional<SpriteRenderer> spriteRenderer;
	std::optional<Rigidbody> rigidbody;
	std::optional<Staticbody> staticbody;

	// use .prefab extension
	void serializeJson(std::string filepath) {
		nlohmann::json ja;
		nlohmann::json j;


		j["behaviorHash"] = behaviorHash;

		transform.position = glm::vec2(0.0f);
		j["transform"] = transform.serializeJson();

		if (colorRenderer.has_value())
			j["colorRenderer"] = colorRenderer.value().serializeJson(0);
		if (spriteRenderer.has_value())
			j["spriteRenderer"] = spriteRenderer.value().serializeJson(0);
		if (rigidbody.has_value())
			j["rigidbody"] = rigidbody.value().serializeJson(0);
		if (staticbody.has_value())
			j["staticbody"] = staticbody.value().serializeJson(0);

		ja["prefab"] = j;

		std::ofstream output(filepath);
		output << ja.dump(4) << std::endl;
		output.close();
	};

	static Prefab deserializeJson(std::string filepath, std::shared_ptr<b2World> world ) {
		std::ifstream input(filepath);
		nlohmann::json ja;
		input >> ja;
		
		nlohmann::json j = ja["prefab"];

		Prefab p;

		p.behaviorHash = j["behaviorHash"];
		p.transform = Transform::deserializeJson(j["transform"]);
		
		if (j.contains("colorRenderer"))
			p.colorRenderer = ColorRenderer::deserializeJson(j["colorRenderer"]);
		if (j.contains("spriteRenderer"))
			p.spriteRenderer = SpriteRenderer::deserializeJson(j["spriteRenderer"]);
		if (j.contains("rigidbody"))
			p.rigidbody = Rigidbody::deserializeJson(j["rigidbody"], world);
		if (j.contains("staticbody"))
			p.staticbody = Staticbody::deserializeJson(j["staticbody"], world);

		return p;
	}
};