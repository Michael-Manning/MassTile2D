#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "Constants.h"

template <typename...> class BindingManager;

// Note, class does not clear map. If different binding IDs are continously added and removed, memory usage will increase

template<typename I, typename T>
class BindingManager<I, T> {

public:

	BindingManager(int maxEntries) : maxEntries(maxEntries) {

		values.resize(maxEntries);
		slotAvailable.resize(maxEntries);
		for (size_t i = 0; i < maxEntries; i++) {
			slotAvailable[i] = true;
		}
	};

	// map a binding in the next availble slot of the vector. Returns slot index
	int AddBinding(I ID, T value) {

		// attempting to bind duplicate ID is acceptable, but redundant
		if (boundIDs.contains(ID)) {
			return bindIndexes[ID];
		}

		assert(bindingCount < maxEntries);

		int foundSlot = -1;
		for (size_t i = 0; i < maxEntries; i++)
		{
			// available slot
			if (slotAvailable[i]) {
				values[i] = std::pair(ID, value);
				bindIndexes[ID] = i;
				slotAvailable[i] = false;
				foundSlot = i;
				goto found;
			}
		}

		assert(false); // somehow did not find available spot

	found:;
		bindingCount++;
		boundIDs.insert(ID);

		// both descriptor sets are now out of date
		InvalidateDescriptors();

		return foundSlot;
	};

	void RemoveBinding(I ID) {

		assert(boundIDs.contains(ID));

		for (size_t i = 0; i < maxEntries; i++)
			if (values[i].first == ID)
				slotAvailable[i] = true;

		boundIDs.erase(ID);

		InvalidateDescriptors();
	};

	void ClearBindings() {
		for (size_t i = 0; i < maxEntries; i++)
			slotAvailable[i] = true;

		boundIDs.clear();
		InvalidateDescriptors();
	};

	void InvalidateDescriptors() {
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
			descriptorDirtyFlags[i] = true;
	};

	bool HasBinding(I ID) {
		return boundIDs.contains(ID);
	}

	T getValueFromIndex(int index) {
		return values[index].second;
	};
	int getIndexFromBinding(I key) {
		return bindIndexes[key];
	};

	T getValueFromBinding(I key) {
		return values[getIndexFromBinding(key)].second;
	};

	bool IsDescriptorDirty(int frame) {
		return descriptorDirtyFlags[frame];
	};

	void ClearDescriptorDirty(int frame) {
		descriptorDirtyFlags[frame] = false;
	};

	bool IsSlotInUse(int index) {
		return !slotAvailable[index];
	};

	const int maxEntries;

private:

	std::unordered_map<texID, int> bindIndexes;
	std::array<bool, FRAMES_IN_FLIGHT> descriptorDirtyFlags = { true, true };
	std::vector<bool> slotAvailable;
	std::vector<std::pair<I, T>> values;

	std::set<I> boundIDs;
	int bindingCount = 0;
};
