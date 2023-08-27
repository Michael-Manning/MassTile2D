#pragma once

#include <cassert>
#include <type_traits>
#include <set>
#include <string>

//#include <functional>
//#include <iomanip>
//#include <iostream>

template <typename T>
class IDGenerator {
public:

	static_assert(std::is_arithmetic_v<T>, "IDGenerator requires a numerical type.");

	T GenerateID() {
		T id = ++lastID;
		generatedIDs.insert(id);
		return id;
	};

	T GenerateID(std::string str) {
		std::size_t res = std::hash<std::string>{}(str);
		T tres = static_cast<T>(res);

		assert(generatedIDs.contains(tres) == false);
		generatedIDs.insert(tres);

		return tres;
	};

	// mark ID as in use
	void Input(T ID) {

		// supplying ID numbers is dangerous. Check for duplication during debug
		assert(generatedIDs.contains(ID) == false);

		generatedIDs.insert(ID);
		if (ID > lastID) {
			lastID = ID;
		}
	}

	T ContainsHash(std::string str) {
		std::size_t res = std::hash<std::string>{}(str);
		T tres = static_cast<T>(res);
		if (generatedIDs.contains(tres)) {
			return tres;
		}
		else {
			return static_cast<T>(0);
		}
	}

	bool Contains(T) {
		return generatedIDs.contains(T);
	}

	void Reset() {
		lastID = 0;
		generatedIDs.clear();
	}

private:
	T lastID = 0;
	std::set<T> generatedIDs;
};