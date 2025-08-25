#pragma once

#include <set>

// has no error checking or validation. Please use responsibly 

template<typename T>
class SlotManager {
public:

    static_assert(std::is_arithmetic_v<T>, "SlotManager requires a numerical type.");

    SlotManager(size_t maxSlots) : maxSlots(maxSlots), nextAvailableSlot(0) {
    }

    T GetAvailableSlot() {
        // If there are freed slots available, use one of those.
        if (!freedSlots.empty()) {
            auto it = freedSlots.begin();
            int slot = *it;
            freedSlots.erase(it);
            return slot;
        }

        // If no freed slots are available, use the next available slot.
        if (nextAvailableSlot < maxSlots) {
            return nextAvailableSlot++; 
        }

        assert(false);
        return 0;
    }

    void FreeSlot(T slot) {
        assert(freedSlots.contains(slot) == false);
        freedSlots.insert(slot);
    }

private:
    size_t maxSlots;
    size_t nextAvailableSlot; // Tracks the next available slot.

    std::set<T> freedSlots; // Tracks freed (or available) slots.
};