#pragma once

#include <vector>
#include <stdint.h>
#include <stdexcept>

#ifndef SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
#define SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
#endif

#include <spirv_reflect.h>
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

	struct spec_constant_info {
		uint32_t constantID;
		uint32_t size;
	};

	static void GetShaderBufferBindings(
		const std::vector<uint8_t>& shaderSrc, 
		std::vector<buffer_info>& bufferInfos, 
		push_constant_info& pushInfo, 
		std::vector<spec_constant_info>& specInfos
	) {

		SpvReflectShaderModule spvModule;
		SpvReflectResult result = spvReflectCreateShaderModule(shaderSrc.size(), reinterpret_cast<const uint32_t*>(shaderSrc.data()), &spvModule);
		if (result != SPV_REFLECT_RESULT_SUCCESS) {
			throw std::runtime_error("Shader reflection failed");
			return;
		}

		// buffer bindings
		{

			uint32_t bindingCount = 0;
			result = spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, nullptr);
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw std::runtime_error("Shader reflection failed");
				spvReflectDestroyShaderModule(&spvModule);
				return;
			}

			if (bindingCount > 0) {
				std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);
				spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, bindings.data());

				for (const auto& binding : bindings) {
					// Filter out if this is not a buffer
					if (binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
						binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
						continue;
					}

					bufferInfos.push_back(buffer_info{
						.set = binding->set,
						.binding = binding->binding,
						.type = binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? vk::DescriptorType::eUniformBuffer : vk::DescriptorType::eStorageBuffer
						});
				}
			}
		}

		// push constant
		{
			pushInfo.size = 0;

			uint32_t blockCount = 0;
			result = spvReflectEnumeratePushConstantBlocks(&spvModule, &blockCount, nullptr);
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw std::runtime_error("Shader reflection failed");
				spvReflectDestroyShaderModule(&spvModule);
				return;
			}

			if (blockCount > 0) {
				std::vector<SpvReflectBlockVariable*> pushConstants(blockCount);
				result = spvReflectEnumeratePushConstantBlocks(&spvModule, &blockCount, pushConstants.data());
				if (result != SPV_REFLECT_RESULT_SUCCESS) {
					throw std::runtime_error("Shader reflection failed");
					spvReflectDestroyShaderModule(&spvModule);
					return;
				}

				for (const auto& block : pushConstants) {

					// only support one push constant per pipeline
					assert(pushInfo.size == 0);

					pushInfo.size = block->size;
				}
			}
		}

		// Specialization constants (assuming they could be input variables)
		{
			uint32_t variableCount = 0;
			result = spvReflectEnumerateInterfaceVariables(&spvModule, &variableCount, nullptr);
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw std::runtime_error("Failed to enumerate input variables");
				spvReflectDestroyShaderModule(&spvModule);
				return;
			}

			std::vector<SpvReflectInterfaceVariable*> variables(variableCount);
			result = spvReflectEnumerateInterfaceVariables(&spvModule, &variableCount, variables.data());
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw std::runtime_error("Failed to enumerate input variables");
				spvReflectDestroyShaderModule(&spvModule);
				return;
			}

			for (const auto& variable : variables) {

				int bbreakl = 0;

				//if (variable->decoration_flags & SPV_REFLECT_DECORATION_SPEC_ID) {
				//	specInfos.push_back(spec_constant_info{
				//		.location = variable->location,
				//		.size = variable->numeric.scalar.width / 8 // Assuming 8 bits per byte
				//		});
				//}
			}
		}


		spvReflectDestroyShaderModule(&spvModule);
	}
}