#pragma once

#include <vector>
#include <string>
#include <stdint.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include "texture.h"

namespace VKUtil{

	std::vector<char> readFile(const std::string& filename);

	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);
}