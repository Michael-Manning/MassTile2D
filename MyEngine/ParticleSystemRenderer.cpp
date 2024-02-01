#include "stdafx.h"

#include <random>
#include <glm/glm.hpp>

#include "ParticleSystem.h"
#include "ParticleSystemPL.h"
#include "ParticleSystemRenderer.h"


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

void ParticleSystemRenderer::runSimulation(float deltaTime, ParticleSystem& ps, glm::vec2 spawnOrigin) {


	assert(particleData.particleCount == ps.particleCount);

	spawntimer += deltaTime;

	int particlesToSpawn = 0;

	// math is hard
	float step = (1.0f / ps.spawnRate);
	while (spawntimer >= step) {
		particlesToSpawn++;
		spawntimer -= step;
	}

	for (size_t i = 0; i < ps.particleCount; i++)
	{
		particleData.particles[i].scale = 0.2f;

		particleData.particles[i].life -= deltaTime;

		particleData.particles[i].position += particleData.particles[i].velocity * deltaTime;

		if (particlesToSpawn > 0 && particleData.particles[i].life <= 0.0f) {
			
			particleData.particles[i].position = spawnOrigin;
			particleData.particles[i].velocity = (randomNormal2() - 0.5f) * 4.0f;
			particleData.particles[i].life = 1.0f;
			particlesToSpawn--;
		}
	}
}