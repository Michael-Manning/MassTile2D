#pragma once

#include <vector>
#include <stdint.h>
#include <stdexcept>

//#ifndef SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
//#define SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
//#endif

#include <vulkan/vulkan.hpp>

namespace Reflection {

	struct buffer_info {
		uint32_t set;
		uint32_t binding;
		vk::DescriptorType type;
	};

	struct push_constant_info {
		uint32_t size;
	};

	void GetShaderBindings(const std::vector<uint8_t>& shaderSrc, std::vector<buffer_info>& bufferInfos, push_constant_info& pushInfo);
}