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

#include "typedefs.h"


namespace ImGui {
	static void InputString(const char * label, std::string& str) {
		char buffer[256];
		std::fill(buffer, buffer + 255, '\0');
		std::copy(str.c_str(), str.c_str() + str.length(), buffer);
		ImGui::InputText(label, buffer, 256);
		str = std::string(buffer);
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