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

	void selectedItemChange(State& state) {
		auto selected = &global::playerInventory.slots.at(state.selectedHotBarSlot);
		global::player->SetSelectedItem(selected->InUse() ? selected : nullptr);
	}

	void drawInventorySprite(State& state, vec2 position, const ItemBase& base) {

		assert(base.type != ItemBase::Type::Empty);
		state.engine->addScreenCenteredSpaceTexture(base.sprite, base.atlasIndex, position, InvSlotTextureSize);
		
		
		//if (base.type == ItemBase::Type::Tool || base.type == ItemBase::Type::Consumable) {
		//	//state.engine->addScreenCenteredSpaceTexture("itemSprites", base.inventorySpriteAtlasIndex, position, InvSlotTextureSize);
		//	//state.engine->addScreenCenteredSpaceTexture(base.sprite, base.atlasIndex, position, InvSlotTextureSize);
		//}
		//else if(base.type == ItemBase::Type::Block){
		//	//state.engine->addScreenCenteredSpaceTexture(base.sprite, base.atlasIndex, position, InvSlotTextureSize);
		//	//state.engine->addScreenCenteredSpaceTexture("tilemapSprites", base.inventorySpriteAtlasIndex, position, InvSlotTextureSize);
		//}
		//else {
		//	assert(false);
		//}
	}

	void HandleMouseInputForSlot(State& state, InventoryContainer* container, int slot) {

		auto input = state.engine->GetInput();
		if (input->getMouseBtnDown(MouseBtn::Left)) {
			// move as much of cursor stack that will fit into inventory
			if (global::cursorInventory.slots[0].InUse()) {
				if (container->slots[slot].InUse() == false || container->slots[slot].item == global::cursorInventory.slots[0].item) {

					int count = global::cursorInventory.slots[0].count;
					if (container->slots[slot].InUse() == true)
						count = glm::min(count, itemLibrary.GetItem(container->slots[slot].item).maxStack - container->slots[slot].count);

					Inventory::MoveMergeStack(&global::cursorInventory, 0, container, slot, count);

					state.dragSrcContainer = nullptr;
					state.dragSrcSlot = -1;

					if (container == &global::playerInventory && slot == state.selectedHotBarSlot)
						selectedItemChange(state);
				}
			}
			// move whole cursor stack into cursor
			else if (container->slots[slot].InUse() == true) {

				state.dragSrcContainer = container;
				state.dragSrcSlot = slot;

				Inventory::MoveStack(state.dragSrcContainer, state.dragSrcSlot, &global::cursorInventory, 0);

				if (container == &global::playerInventory && slot == state.selectedHotBarSlot)
					selectedItemChange(state);
			}
		}
		else if (input->getMouseBtnDown(MouseBtn::Right)) {
			// move up to one item from cursor stack into inventory
			if (global::cursorInventory.slots[0].InUse()) {
				if (container->slots[slot].InUse() == false || container->slots[slot].item == global::cursorInventory.slots[0].item) {

					int count = 1;
					if (container->slots[slot].InUse() == true)
						count = glm::min(count, itemLibrary.GetItem(container->slots[slot].item).maxStack - container->slots[slot].count);

					Inventory::MoveMergeStack(&global::cursorInventory, 0, container, slot, count);

					// moved whole cursor stack
					if (global::cursorInventory.slots[0].count == 0) {
						state.dragSrcContainer = nullptr;
						state.dragSrcSlot = -1;
					}

					if (container == &global::playerInventory && slot == state.selectedHotBarSlot)
						selectedItemChange(state);
				}
			}
			// move half the stack into curosr
			else if (container->slots[slot].InUse() == true) {

				state.dragSrcContainer = container;
				state.dragSrcSlot = slot;

				int count = static_cast<int>((float)container->slots[slot].count / 2.0f + 0.5f);

				Inventory::MoveMergeStack(state.dragSrcContainer, state.dragSrcSlot, &global::cursorInventory, 0, count);

				if (container == &global::playerInventory && slot == state.selectedHotBarSlot)
					selectedItemChange(state);
			}
		}
	}



	void DoUI(State& state) {

		auto input = state.engine->GetInput();
		auto mpos = input->getMousePos();

		if (input->getKeyDown('i')) {
			state.showingInventory = !state.showingInventory;
			if (state.showingInventory == false) {
				global::inspectedInventory = nullptr;
			}
		}
		if (input->getKeyDown(KeyCode::Escape)) {
			state.showingInventory = false;
			global::inspectedInventory = nullptr;

		}

		if (global::inspectedInventory != nullptr) {
			state.showingInventory = true;
		}


		for (char c = '0'; c <= '9'; c++)
		{
			if (input->getKeyDown(c)) {
				int slot = numRowToHotbarIndex(c - '0');
				if (slot != state.selectedHotBarSlot) {
					state.selectedHotBarSlot = numRowToHotbarIndex(c - '0');
					selectedItemChange(state);
				}
			}
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


				HandleMouseInputForSlot(state, &global::playerInventory, i);

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
				//state.engine->addScreenCenteredSpaceTexture("itemSprites", itemHeader.inventorySpriteAtlasIndex, pos + InvSlotSize / 2.0f, InvSlotTextureSize);
				drawInventorySprite(state, pos + InvSlotSize / 2.0f, itemHeader);
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
			//state.engine->addScreenCenteredSpaceTexture("itemSprites", itemHeader.inventorySpriteAtlasIndex, mpos, InvSlotTextureSize);

			drawInventorySprite(state, mpos, itemHeader);

			if (itemHeader.maxStack > 1) {
				state.engine->addScreenSpaceText(state.smallfont, mpos - vec2(InvSlotSize / 4.0f, 0.0f), vec4(1.0), to_string(global::cursorInventory.slots[0].count));
			}
		}


		// not great spot for this check
		if (global::player != nullptr && global::inspectedInventory != nullptr && glm::distance(global::player->GetEntity()->transform.position, global::inspectedInventoryLocation) > maxInspectedInventoryViewDistance)
			global::inspectedInventory = nullptr;

		if (global::inspectedInventory == nullptr)
			return;

		int invRows = global::inspectedInventory->size / inspectedInventoryRows + (int)(global::inspectedInventory->size % inspectedInventoryRows > 0);
		int slots = global::inspectedInventory->size;
		vec2 startPos = vec2(invPos.x, state.engine->winH - (invRows * InvSlotSize) - (invRows * InvSlotGap) + InvSlotGap);
		for (size_t i = 0; i < global::inspectedInventory->size; i++)
		{
			vec2 slotPos = startPos + vec2((i % inspectedInventoryRows) * (InvSlotSize + InvSlotGap), (i / inspectedInventoryRows) * (InvSlotSize + InvSlotGap));

			state.engine->addScreenSpaceTexture("invSlot", 0, slotPos, InvSlotSize);

			if (global::inspectedInventory->slots[i].InUse()) {
				const auto& itemHeader = itemLibrary.GetItem(global::inspectedInventory->slots[i].item);
				//state.engine->addScreenCenteredSpaceTexture("itemSprites", itemHeader.inventorySpriteAtlasIndex, slotPos + InvSlotSize / 2.0f, InvSlotTextureSize);
				
				drawInventorySprite(state, slotPos + InvSlotSize / 2.0f, itemHeader);
				
				if (itemHeader.maxStack > 1) {
					state.engine->addScreenSpaceText(state.smallfont, slotPos + vec2(InvSlotSize / 2.0f - InvSlotSize / 4.0f, InvSlotSize / 2.0f), vec4(1.0), to_string(global::inspectedInventory->slots[i].count));
				}
			}

			if (within(slotPos - InvSlotGap / 1.5f, slotPos + InvSlotSize + InvSlotGap / 1.5f, mpos)) {

				HandleMouseInputForSlot(state, global::inspectedInventory, i);


				state.engine->addScreenSpaceCenteredQuad(glm::vec4(1, 1, 1, 0.2f), slotPos + InvSlotSize / 2.0f, vec2(InvSlotSize));
			}
		}
	}
}