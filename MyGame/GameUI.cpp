#include "stdafx.h"
#include <glm/glm.hpp>


#include "Utils.h"
#include "GameUI.h"

#include "global.h"

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

		for (char c = '0'; c <= '9'; c++)
		{
			if (input->getKeyDown(c))
				state.selectedHotBarSlot = numRowToHotbarIndex(c - '0');
		}

		state.engine->addScreenSpaceTexture("hotbar", 0, hotBarPos, 127.0f * UIScale);
		if (state.showingInventory) {
			state.engine->addScreenSpaceTexture("inventory", 0, invPos, 550.0f * UIScale);
		}

		{
			auto pos = getInvSlotPos(state.selectedHotBarSlot);
			state.engine->addScreenSpaceCenteredQuad(glm::vec4(0.3, 0.3, 1.0, 0.3f), pos + InvSlotSize / 2.0f, vec2(InvSlotSize));
		}

		for (size_t i = 0; i < hotBarSlots + invSlotCount; i++) {
			if (i >= hotBarSlots && state.showingInventory == false)
				break;

			auto pos = getInvSlotPos(i);

			// using 1.5 because sometimes rounding causes gaps to not select anything
			if (within(pos - InvSlotGap / 1.5f, pos + InvSlotSize + InvSlotGap / 1.5f, mpos)) {





				if (input->getMouseBtnDown(MouseBtn::Left)) {
					// move as much of cursor stack that will fit into inventory
					if (global::cursorInventory.slots[0].InUse()) {
						if (global::playerInventory.slots[i].InUse() == false || global::playerInventory.slots[i].item == global::cursorInventory.slots[0].item) {

							int count = global::cursorInventory.slots[0].count;
							if (global::playerInventory.slots[i].InUse() == true)
								count = glm::min(count, itemLibrary.GetItem(global::playerInventory.slots[i].item).maxStack - global::playerInventory.slots[i].count);

							Inventory::MoveMergeStack(&global::cursorInventory, 0, &global::playerInventory, i, count);

							state.dragSrcContainer = nullptr;
							state.dragSrcSlot = -1;
						}
					}
					// move whole cursor stack into cursor
					else if (global::playerInventory.slots[i].InUse() == true) {

						state.dragSrcContainer = &global::playerInventory;
						state.dragSrcSlot = i;

						Inventory::MoveStack(state.dragSrcContainer, state.dragSrcSlot, &global::cursorInventory, 0);
					}
				}
				else if (input->getMouseBtnDown(MouseBtn::Right)) {
					// move up to one item from cursor stack into inventory
					if (global::cursorInventory.slots[0].InUse()) {
						if (global::playerInventory.slots[i].InUse() == false || global::playerInventory.slots[i].item == global::cursorInventory.slots[0].item) {

							int count = 1;
							if (global::playerInventory.slots[i].InUse() == true)
								count = glm::min(count, itemLibrary.GetItem(global::playerInventory.slots[i].item).maxStack - global::playerInventory.slots[i].count);

							Inventory::MoveMergeStack(&global::cursorInventory, 0, &global::playerInventory, i, count);

							// moved whole cursor stack
							if (global::cursorInventory.slots[0].count == 0) {
								state.dragSrcContainer = nullptr;
								state.dragSrcSlot = -1;
							}
						}
					}
					// move half the stack into curosr
					else if (global::playerInventory.slots[i].InUse() == true) {

						state.dragSrcContainer = &global::playerInventory;
						state.dragSrcSlot = i;

						int count = static_cast<int>((float)global::playerInventory.slots[i].count / 2.0f + 0.5f);

						Inventory::MoveMergeStack(state.dragSrcContainer, state.dragSrcSlot, &global::cursorInventory, 0, count);
					}

				}


				state.engine->addScreenSpaceCenteredQuad(glm::vec4(1, 1, 1, 0.2f), pos + InvSlotSize / 2.0f, vec2(InvSlotSize));
				break;
			}

		}

		for (size_t i = 0; i < hotBarSlots + invSlotCount; i++) {

			if (i >= hotBarSlots && state.showingInventory == false)
				break;

			if (global::playerInventory.slots[i].InUse()) {
				auto pos = getInvSlotPos(i);
				const auto& itemHeader = itemLibrary.GetItem(global::playerInventory.slots[i].item);
				state.engine->addScreenCenteredSpaceTexture("itemSprites", itemHeader.inventorySpriteAtlasIndex, pos + InvSlotSize / 2.0f, InvSlotTextureSize);
				if (itemHeader.maxStack > 1) {
					state.engine->addScreenSpaceText(state.smallfont, pos + vec2(InvSlotSize / 2.0f - InvSlotSize / 4.0f, InvSlotSize / 2.0f), vec4(1.0), to_string(global::playerInventory.slots[i].count));
				}
			}
		}

		if (state.dragSrcContainer != nullptr && (input->getKeyDown(KeyCode::Escape) || input->getKeyDown('i'))) {
			Inventory::MoveMergeStack(&global::cursorInventory, 0, state.dragSrcContainer, state.dragSrcSlot, global::cursorInventory.slots[0].count);
			state.dragSrcContainer = nullptr;
			state.dragSrcSlot = -1;
		}

		//if (input->getMouseBtnUp(MouseBtn::Left) && state.dragSrcContainer) {
		//	Inventory::MoveStack(&global::cursorInventory, 0, state.dragSrcContainer, state.dragSrcSlot);
		//	state.dragSrcContainer = nullptr;
		//	state.dragSrcSlot = -1;
		//}

		//if (state.dragSrcContainer != nullptr) {
		if (global::cursorInventory.slots[0].InUse()) {
			vec2 mpos = input->getMousePos();
			const auto& itemHeader = itemLibrary.GetItem(global::cursorInventory.slots[0].item);
			state.engine->addScreenCenteredSpaceTexture("itemSprites", itemHeader.inventorySpriteAtlasIndex, mpos, InvSlotTextureSize);
			if (itemHeader.maxStack > 1) {
				state.engine->addScreenSpaceText(state.smallfont, mpos - vec2(InvSlotSize / 4.0f, 0.0f), vec4(1.0), to_string(global::cursorInventory.slots[0].count));
			}
		}
	}
}