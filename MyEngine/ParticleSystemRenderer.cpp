#include "stdafx.h"

#include <random>
#include <glm/glm.hpp>

#include "ParticleSystemPL.h"
#include "ParticleSystemRenderer.h"
#include "MyMath.h"

namespace {
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator

	int randomRange(int min, int max) {
		std::uniform_int_distribution<> dis(min, max);
		return dis(gen);
	}

	inline float randomNormal() {
		return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	}

	inline glm::vec2 randomNormal2() {
		return glm::vec2(randomNormal(), randomNormal());
	}

	enum class AppState {
		SplashScreen,
		MainMenu,
		PlayingGame
	};

	AppState appState = AppState::PlayingGame;
}

void ParticleSystemRenderer::runSimulation(float deltaTime, glm::vec2 spawnOrigin) {

	// only small size system are for host simulation
	assert(size == Size::Small);

	const auto& con = configuration;
	auto& ps = hostParticleBuffer->particles;

	if (configuration.burstMode && spawntimer == 0.0f && deltaTime > 0) {
		for (size_t id = 0; id < con.particleCount; id++) {
			ps[id].life = 1.0f;
			ps[id].position = spawnOrigin;
			//ps[id].velocity = (randomNormal2() - 0.5f) * lerp(con.spawnMinVelocity, con.spawnMaxVelocity, randomNormal());
			ps[id].velocity = (randomNormal2() - 0.5f) * 4.0f;
			particlesToSpawn--;
		}
	}

	spawntimer += deltaTime;



	// math is hard
	if (configuration.burstMode == false) {
		float step = (1.0f / con.spawnRate);
		particlesToSpawn = static_cast<int>(spawntimer / step);
		spawntimer -= particlesToSpawn * step;
	}
	else {
		if (spawntimer > configuration.particleLifeSpan && configuration.burstRepeat) {
			spawntimer = 0.0f;
			return;
		}
	}

	for (size_t id = 0; id < con.particleCount; id++)
	{
		if (configuration.burstMode == false && (particlesToSpawn > 0 && ps[id].life <= 0.0f)) {
			ps[id].life = 1.0f;
			ps[id].position = spawnOrigin;
			//ps[id].velocity = (randomNormal2() - 0.5f) * lerp(con.spawnMinVelocity, con.spawnMaxVelocity, randomNormal());
			ps[id].velocity = (randomNormal2() - 0.5f) * 4.0f;
			particlesToSpawn--;
		}

		else {
			ps[id].life -= deltaTime / con.particleLifeSpan;
			ps[id].scale = lerp(con.startSize, con.endSize, 1.0 - ps[id].life);
			ps[id].color = lerp(con.startColor, con.endColor, 1.0 - ps[id].life);
			ps[id].velocity.y += con.gravity * deltaTime;
			ps[id].position += ps[id].velocity * deltaTime;
		}
	}

}

nlohmann::json ParticleSystemRenderer::serializeJson(entityID ID) const {
	nlohmann::json j;

	j["entityID"] = ID;
	j["size"] = size;
	j["configuration"] = configuration.serializeJson();

	return j;
}

ParticleSystemRenderer::ParticleSystemRenderer(nlohmann::json& j) : configuration(j["configuration"]){
	size = j["size"];
	//ParticleSystemPL::ParticleSystemConfiguration::deserializeJson(j["configuration"], &configuration);
	SetSystemSize(size);
}
ParticleSystemRenderer::ParticleSystemRenderer(const AssetPack::ParticleSystemRenderer* p) : configuration(&p->configuration()) {
	size = static_cast<Size>(p->size());
	//ParticleSystemConfiguration::deserializeFlatbuffers(&p->configuration(), &configuration);
	SetSystemSize(size);
}