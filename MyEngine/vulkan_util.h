#pragma once

#include <vector>
#include <string>
#include <stdint.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include "texture.h"
#include "VKEngine.h"

namespace VKUtil{

	std::vector<char> readFile(const std::string& filename);

	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

	template<typename T>
	class UBOUploader {
	public:

		void CreateBuffers(std::shared_ptr<VKEngine> engine) {
			engine->createMappedBuffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, transferBuffers);
		};

		void Invalidate() {
			for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
				uploadDirtyFlags[i] = true;
		};

		void SyncBufferData(T& data, int currentFrame) {
			if (uploadDirtyFlags[currentFrame] == true) {
				memcpy(transferBuffers.buffersMapped[currentFrame], &data, sizeof(T));
				uploadDirtyFlags[currentFrame] = false;
			}
		}

		MappedDoubleBuffer transferBuffers;

		std::array<bool, FRAMES_IN_FLIGHT> uploadDirtyFlags = { true, true };
	};
}