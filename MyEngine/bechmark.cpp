
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <typeinfo>

#include "VKEngine.h"
#include "Engine.h"
#include "Physics.h"

#include<glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <imgui.h>

#include "Editor.h"
#include "MyMath.h"

#include "BehaviorRegistry.h"

using namespace std;
using namespace glm;

void RunBenchmark(Engine& engine) {

	const auto& scene = engine.scene;

	//auto sprb = scene->GenerateSprite("../data/test_cat.png");

	const int objCount = 100000;


	int w = sqrt(objCount);
	int h = sqrt(objCount);
	for (size_t i = 0; i < w; i++)
	{
		for (size_t j = 0; j < h; j++)
		{
			{
				auto qb = make_shared<Entity>("benchobj", true);
				qb->transform.position = vec2(i, j);
				qb->transform.scale = vec2(1.0, 1.0);
				scene->RegisterEntity(qb);
				scene->registerComponent(qb->ID, SpriteRenderer(2));
			}
		}
	}
}

