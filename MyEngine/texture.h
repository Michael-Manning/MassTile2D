#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

struct Texture {
	vk::Image textureImage;
	VmaAllocation textureImageAllocation;
	vk::ImageView imageView;
	int resolutionX;
	int resolutionY;

	vk::Sampler sampler; // cam be shared between textures.

	std::optional<vk::DescriptorSet> imTexture;
};

enum class FilterMode {
	Nearest,
	Linear
};