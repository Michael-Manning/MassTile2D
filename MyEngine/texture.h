#pragma once

#include <optional>
#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

struct Texture {
	VkImage textureImage;
	VmaAllocation textureImageAllocation;
	VkImageView imageView;
	int resolutionX;
	int resolutionY;

	VkSampler sampler; // cam be shared between textures.

	std::optional<VkDescriptorSet> imTexture;
};