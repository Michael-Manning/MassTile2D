#pragma once

#include "Tool.h"
#include "Collision.h"
#include "Input.h"
#include "global.h"
#include "TileWorld.h"

class Pickaxe : public Tool {

private:


public:
	Pickaxe() {

	}

	void Update(const PlayerCtx& ctx) override {

		if (global::input->getMouseBtn(MouseBtn::Left)) {
			glm::ivec2 tile = GetMouseTile();

			if (tile.x > 1 && tile.x < LargeTileWorldWidth - 1 && tile.y > 1 && tile.y < LargeTileWorldHeight - 1) {


				tileID tileid = global::tileWorld->getTile(tile);

				if (IsMineable(tileid)) {

					blockID block = GetBlock(tileid);
					global::tileWorld->setTile(tile, Blocks::Air);
					UpdateTextureVariations(tile, global::tileWorld);


					// place block into inventory

					itemID blockItemID = itemLibrary.GetBlockItemID(block);
					auto base = itemLibrary.GetItem(blockItemID);

					// first pass find existing stacks to put on top of
					bool inserted = false;
					for (size_t i = 0; i < global::playerInventory.size; i++) {
						if (global::playerInventory.slots[i].InUse() && global::playerInventory.slots[i].item == blockItemID && global::playerInventory.slots[i].count < base.maxStack) {
							global::playerInventory.slots[i].count++;
							inserted = true;
							break;
						}
					}
					if (inserted == false) {
						for (size_t i = 0; i < global::playerInventory.size; i++)
						{
							if (global::playerInventory.slots[i].InUse() == false) {
								ItemStack stack(blockItemID, 1);
								global::playerInventory.slots[i] = stack;
								break;
							}
						}
					}
				}

			}
		}
	}
};