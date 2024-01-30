#include "stdafx.h"
#include <glm/glm.hpp>


#include "Utils.h"
#include "GameUI.h"

using namespace glm;
using namespace std;

namespace {


}

namespace UI {

	/*vec2 getHotBarSlotPos(int slot) {
		return hotBarPos + vec2(slot * (InvSlotSize + InvSlotGap), 0);
	}*/

	vec2 getInvSlotPos(int slot) {
		if (slot < hotBarSlots)
			return hotBarPos + vec2(slot * (InvSlotSize + InvSlotGap), 0);
		slot -= hotBarSlots;
		return invPos + vec2((slot % invSlotsX) * (InvSlotSize + InvSlotGap), (slot / invSlotsX) * (InvSlotSize + InvSlotGap));
	}
	inline int numRowToHotbarIndex(int num) {
		return num == 0 ? 9 : num - 1;
	}


	void DoUI(State& state) {

		auto input = state.engine->GetInput();
		auto mpos = input->getMousePos();

		if (input->getKeyDown('i'))
			state.showingInventory = !state.showingInventory;

		for (char c = '0'; c <='9'; c++)
		{
			if(input->getKeyDown(c))
				state.selectedHotBarSlot = numRowToHotbarIndex(c - '0');
		}

		state.engine->addScreenSpaceTexture("hotbar", 0, hotBarPos, 127.0f * UIScale);
		if (state.showingInventory) {
			state.engine->addScreenSpaceTexture("inventory", 0, invPos, 550.0f * UIScale);
		}
			
		{
			auto pos = getInvSlotPos(state.selectedHotBarSlot);
			state.engine->addScreenSpaceQuad(glm::vec4(1, 1, 1, 0.5f), pos + InvSlotSize / 2.0f, vec2(InvSlotSize));
		}

		for (size_t i = 0; i < hotBarSlots + invSlotCount; i++) {
			if (i >= hotBarSlots && state.showingInventory == false)
				break;

			auto pos = getInvSlotPos(i);
			if (within(pos, pos + InvSlotSize, mpos)) {
				state.engine->addScreenSpaceQuad(glm::vec4(1, 1, 1, 0.5f), pos + InvSlotSize / 2.0f, vec2(InvSlotSize));
				break;
			}

		}

	}

}