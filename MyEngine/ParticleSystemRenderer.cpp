#include "stdafx.h"

#include <random>
#include <glm/glm.hpp>

#include "ParticleSystem.h"
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
	assert(size == ParticleSystemSize::Small);

	const auto& con = configuration;
	auto& ps = hostParticleBuffer->particles;

	spawntimer += deltaTime;

	int particlesToSpawn = 0;

	// math is hard
	float step = (1.0f / con.spawnRate);
	while (spawntimer >= step) {
		particlesToSpawn++;
		spawntimer -= step;
	}


	for (size_t i = 0; i < con.particleCount; i++)
	{
		ps[i].life -= deltaTime / con.particleLifeSpan;

		ps[i].scale = lerp(con.startSize, con.endSize, 1.0 - ps[i].life);

		ps[i].color = lerp(con.startColor, con.endColor, 1.0 - ps[i].life);

		// technically incorrect way of applying changing velocity with deltatime
		ps[i].velocity.y += con.gravity * deltaTime;
		ps[i].position += ps[i].velocity * deltaTime;

		if (particlesToSpawn > 0 && ps[i].life <= 0.0f) {
			
			ps[i].position = spawnOrigin;
			ps[i].velocity = (randomNormal2() - 0.5f) * 4.0f;
			ps[i].life = 1.0f;
			particlesToSpawn--;
		}
	}
}