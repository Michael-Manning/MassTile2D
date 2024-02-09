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


	// math is hard
	float step = (1.0f / con.spawnRate);
	int particlesToSpawn = static_cast<int>(spawntimer / step);
	spawntimer -= particlesToSpawn * step;


	for (size_t id = 0; id < con.particleCount; id++)
	{
		if (particlesToSpawn > 0 && ps[id].life <= 0.0f) {
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