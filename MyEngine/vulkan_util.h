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

	VkShaderModule createShaderModule(const std::vector<uint8_t>& code, VkDevice device);

	// mapped double buffer wrapped with frame dirty logic
	template<typename T>
	class BufferUploader {
	public:

		void CreateBuffers(std::shared_ptr<VKEngine> engine, VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
			engine->createMappedBuffer(sizeof(T), usage, transferBuffers);
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
		};

		MappedDoubleBuffer<void> transferBuffers;

		std::array<bool, FRAMES_IN_FLIGHT> uploadDirtyFlags = { true, true };
	};
}