#pragma once

#include <vector>
#include <string>
#include <stdint.h>

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include "texture.h"
#include "VKEngine.h"

namespace VKUtil{

	vk::ShaderModule createShaderModule(const std::vector<uint8_t>& code, vk::Device device);

	// mapped double buffer wrapped with frame dirty logic
	template<typename T>
	class BufferUploader {
	public:

		void CreateBuffers(VKEngine* engine, vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eUniformBuffer) {
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

		MappedDoubleBuffer<T> transferBuffers;

		std::array<bool, FRAMES_IN_FLIGHT> uploadDirtyFlags = { true, true };
	};
}