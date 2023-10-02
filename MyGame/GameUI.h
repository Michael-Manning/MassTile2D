#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "Engine.h"

namespace UI {

	constexpr float UIScale = 0.5f;
	constexpr glm::vec2 hotBarPos = glm::vec2(10) * UIScale;
	constexpr glm::vec2 invPos = glm::vec2(10.0f * UIScale, hotBarPos.y + 26.0f * UIScale + 127.0f * UIScale) ;

	constexpr float InvSlotSize = 127.0f * UIScale;
	constexpr float InvSlotGap = 14.0f * UIScale;

	constexpr int hotBarSlots = 10;
	constexpr int invSlotsX = 10;
	constexpr int invSlotsY = 4;
	constexpr int invSlotCount = invSlotsX * invSlotsY;

	struct State {
		Engine* engine;

		int selectedHotBarSlot = 0;
		bool showingInventory = false;

	};

	void DoUI(State& state);
}

