#include "stdafx.h"

#include <vector>
#include <string>
#include <stdint.h>
#include <fstream>
#include <filesystem>
#include <iostream>

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include "texture.h"

namespace VKUtil {

	vk::ShaderModule createShaderModule(const std::vector<uint8_t>& code, vk::Device device) {
		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		vk::ShaderModule shaderModule;
		shaderModule = device.createShaderModule(createInfo);

		return shaderModule;
	}

}