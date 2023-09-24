#pragma once

#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cassert>
#include <memory>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <string>

// supports types of fixed size and vectors/unordered maps of types of fixed size.
// can write string and const char*, but can only read back as string

constexpr int BinaryWriter_MAX_STRING_SIZE = 256;

class BinaryWriter {
public:
	BinaryWriter(const std::string& filename) {

		outStream = std::make_unique<std::ofstream>(filename, std::ios::binary | std::ios::out);

		if (!outStream->is_open())
			throw std::runtime_error("Failed to open file for writing");
	};

	~BinaryWriter() {
		outStream->close();
	};

	template <typename T>
	BinaryWriter& operator<<(const T& value) {
		write(value);
		return *this;
	};

	BinaryWriter& operator<<(const char * value) {
		write(std::string(value));
		return *this;
	};
	
	BinaryWriter& operator<<(std::string& value) {
		assert(value.length() < BinaryWriter_MAX_STRING_SIZE);
		write((uint32_t)value.length());
		outStream->write(reinterpret_cast<const char*>(value.c_str()), value.length());
		return *this;
	};

	template <typename T>
	BinaryWriter& operator<<(const std::vector<T>& vec) {
		size_t size = vec.size();
		write(size);
		for (const T& value : vec) {
			write(value);
		}
		return *this;
	};

	template <typename I, typename T>
	BinaryWriter& operator<<(const std::unordered_map<I, T>& map) {
		size_t size = map.size();
		write(size);
		for (const auto &[key, value]:map) {
			write(key);
			write(value);
		}
		return *this;
	};

private:
	std::unique_ptr<std::ofstream> outStream;

	template <typename T>
	void write(const T& value) {
		outStream->write(reinterpret_cast<const char*>(&value), sizeof(value));
	};
};

class BinaryReader {
public:

	BinaryReader(const std::string& filename) {

		assert(std::filesystem::exists(filename));

		inStream = std::make_unique<std::ifstream>(filename, std::ios::binary | std::ios::in);

		if (!inStream->is_open())
			throw std::runtime_error("Failed to open file for reading");
	}

	~BinaryReader() {
		inStream->close();
	}

	template <typename T>
	BinaryReader& operator>>(T& value) {
		read(value);
		return *this;
	}

	BinaryReader& operator>>(std::string& value) {
		int size;
		read(size);
		//value = std::string("Example");
		inStream->read(textBuf, size);
		textBuf[size + 1] = '\0';
		value = std::string(&textBuf[0]);
		return *this;
	}

	template <typename T>
	BinaryReader& operator>>(std::vector<T>& vec) {
		size_t size;
		read(size);
		vec.resize(size);
		for (T& value : vec) {
			read(value);
		}
		return *this;
	}

	template <typename I, typename T>
	BinaryReader& operator>>(std::unordered_map<I, T>& map) {
		size_t size;
		read(size);
		for (size_t i = 0; i < size; i++)
		{
			I key;
			read(key);
			T value;
			read(value);
			map[key] = value;
		}
		return *this;
	}

private:
	std::unique_ptr<std::ifstream> inStream;

	char textBuf[BinaryWriter_MAX_STRING_SIZE + 1];

	template <typename T>
	void read(T& value) {
		inStream->read(reinterpret_cast<char*>(&value), sizeof(value));
	}
};