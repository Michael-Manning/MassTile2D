
#include "stdafx.h"

#include <memory>

#include "Engine.h"
#include "Scene.h"
#include "ECS.h"
#include "profiling.h"

using namespace std;
using namespace glm;

namespace Benchmark {

	void BuildSpriteEntityStressTest(Scene* scene, spriteID sprite) {

		const int rows = 100;
		const int cols = 100;

		const float size = 2.0f;
		const float padding = 0.1f;



		PROFILE_START(benchmark_Create_sprite_entities);

		for (size_t i = 0; i < rows; i++)
		{
			for (size_t j = 0; j < cols; j++)
			{
				auto e = scene->CreateEntity();
				e->transform.position = vec2(i * size + i * padding, j * size + j * padding);
				e->transform.scale = vec2(size);
				scene->registerComponent(e->ID, sprite);
			}
		}

		PROFILE_END(benchmark_Create_sprite_entities);

	}

}