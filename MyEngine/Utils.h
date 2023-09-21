#pragma once

#include<string>
#include <imgui.h>


#include <string>
#include <iostream>
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