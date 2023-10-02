#pragma once

#include<string>
#include <imgui.h>


#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <concepts>
#include <stdint.h>
#include <utility>
#include <functional>
#include <memory>
#include <filesystem>
#include <Windows.h>

#include "typedefs.h"


namespace ImGui {
	static bool InputString(const char * label, std::string& str) {
		char buffer[256];
		std::fill(buffer, buffer + 255, '\0');
		std::copy(str.c_str(), str.c_str() + str.length(), buffer);
		bool change = ImGui::InputText(label, buffer, 256);
		str = std::string(buffer);
        return change;
	}

    static bool Combo(const char* label, int* selectedItem, std::vector<std::string> items) {
        char optionStr[256];
        std::fill(optionStr, optionStr + 256, '\0');

        int offset = 0;
        for (auto& s : items) {
            strcpy(optionStr + offset, s.c_str());
            offset += s.length() + 1;
        }

        return Combo(label, selectedItem, optionStr);
    }
}

constexpr uint64_t FNV_prime = 1099511628211u;
constexpr uint64_t FNV_offset_basis = 14695981039346656037u;
constexpr uint32_t cHash(std::string_view str, uint64_t hash = FNV_offset_basis) {
	return static_cast<uint32_t>(str.empty() ? hash : cHash(str.substr(1), (hash ^ static_cast<uint64_t>(str[0])) * FNV_prime));
}


static std::vector<std::string> getAllFilesInDirectory(const std::filesystem::path& path) {
    std::vector<std::string> files;

    // Check if path exists and is a directory.
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (std::filesystem::is_regular_file(entry.path())) {
                files.push_back(std::filesystem::absolute(entry.path()).string());
            }
        }
    }

    return files;
}

static std::filesystem::path get_executable_directory() {
    char buffer[MAX_PATH];
    HMODULE hModule = GetModuleHandle(nullptr);
    if (GetModuleFileName(hModule, buffer, MAX_PATH)) {
        std::filesystem::path exePath(buffer);
        return exePath.parent_path();
    }
    return "";
}

static std::string makePathAbsolute(std::filesystem::path originatingPath, std::string relativePath) {
    // Specifying a relative path
    std::filesystem::path _relativePath(relativePath);

    // Converting to an absolute path
    std::filesystem::path absolutePath = originatingPath / _relativePath;
    absolutePath = std::filesystem::absolute(absolutePath);
    absolutePath = std::filesystem::canonical(absolutePath);  // Normalize the path
    return absolutePath.string();
}

template<typename T>
static int indexOf(const std::vector<T>& v, const T& value) {
    return std::distance(v.begin(), std::find(v.begin(), v.end(), value));
}

static void checkAppend(std::string& str, std::string end) {
    if (str.substr(str.size() - end.length()) != end) {
        str += end;
    }
}

// return file name at end of path
static std::string getFileName(std::string fullPath) {
    std::string name = std::filesystem::path(fullPath).filename().string();
    return name;
}

// return file name at end of path without extension
static std::string getFileRawName(std::string fullPath) {
    std::string name = std::filesystem::path(fullPath).filename().string();
    size_t lastindex = name.find_last_of(".");
    std::string rawname = name.substr(0, lastindex);
    return rawname;
}

static std::vector<uint8_t> readFile(const std::string& filename) {

    if (std::filesystem::exists(std::filesystem::path(filename)) == false) {
        std::cout << "file not found: " << filename << std::endl;
        throw std::runtime_error("failed to open file!");
    }

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint8_t> buffer(fileSize);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    file.close();

    return buffer;
}

static std::vector<uint8_t> readFile(const std::string& filename, int offset, int maxLength) {

    if (std::filesystem::exists(std::filesystem::path(filename)) == false) {
        std::cout << "file not found: " << filename << std::endl;
        throw std::runtime_error("failed to open file!");
    }

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    fileSize -= offset;
    fileSize = fileSize > maxLength ? maxLength : fileSize;
    std::vector<uint8_t> buffer(fileSize);

    file.seekg(offset);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    file.close();

    return buffer;
}


static bool within(glm::vec2 rectStart, glm::vec2  rectEnd, glm::vec2 pos) {
    return pos.x > rectStart.x && pos.x < rectEnd.x && pos.y > rectStart.y && pos.y < rectEnd.y;
}



