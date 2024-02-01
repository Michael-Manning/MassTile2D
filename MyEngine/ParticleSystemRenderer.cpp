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

	spawntimer += deltaTime;

	int particlesToSpawn = 0;

	// math is hard
	float step = (1.0f / particleSystem.configuration.particleCount);
	while (spawntimer >= step) {
		particlesToSpawn++;
		spawntimer -= step;
	}

	const auto& con = particleSystem.configuration;
	const auto& ps = particleSystem.particles;

	for (size_t i = 0; i < particleSystem.configuration.particleCount; i++)
	{
		particleSystem.particles[i].life -= deltaTime;

		particleSystem.particles[i].scale = lerp(con.startSize, con.endSize, 1.0 - ps[i].life);

		particleSystem.particles[i].position += particleSystem.particles[i].velocity * deltaTime;

		if (particlesToSpawn > 0 && particleSystem.particles[i].life <= 0.0f) {
			
			particleSystem.particles[i].position = spawnOrigin;
			particleSystem.particles[i].velocity = (randomNormal2() - 0.5f) * 4.0f;
			particleSystem.particles[i].life = 1.0f;
			particlesToSpawn--;
		}
	}
}