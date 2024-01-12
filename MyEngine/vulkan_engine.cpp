//#define VK_USE_PLATFORM_WIN32_KHR
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <fstream>
#include <chrono>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vk_mem_alloc.h>
#include <tracy/Tracy.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VKEngine.h"
#include "typedefs.h"
#include "Settings.h"

using namespace glm;
using namespace std;


//void VKEngine::createRenderPass() {
//	vk::AttachmentDescription colorAttachment{};
//	colorAttachment.format = swapChainImageFormat;
//	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//	vk::AttachmentReference colorAttachmentRef{};
//	colorAttachmentRef.attachment = 0;
//	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//	vk::SubpassDescription subpass{};
//	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//	subpass.colorAttachmentCount = 1;
//	subpass.pColorAttachments = &colorAttachmentRef;
//
//	vk::RenderPassCreateInfo renderPassInfo{};
//	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//	renderPassInfo.attachmentCount = 1;
//	renderPassInfo.pAttachments = &colorAttachment;
//	renderPassInfo.subpassCount = 1;
//	renderPassInfo.pSubpasses = &subpass;
//
//
//	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
//		throw std::runtime_error("failed to create render pass!");
//	}
//}

void VKEngine::createRenderPass(int subpassCount) {

	assert(subpassCount < 3); // not properly supporting more because of the way the subpass dependencies are set up

	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vector<vk::SubpassDescription> subpasses{};
	subpasses.reserve(subpassCount);
	for (size_t i = 0; i < subpassCount; i++) {
		vk::SubpassDescription subpass{};
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpasses.push_back(subpass);
	}

	vk::SubpassDependency dependency{};
	if (subpassCount > 1) {
		dependency.srcSubpass = 0;
		dependency.dstSubpass = 1;
		dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
	}

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = subpassCount;
	renderPassInfo.pSubpasses = subpasses.data();

	if (subpassCount > 1) {
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;
	}

	renderPass = device.createRenderPass(renderPassInfo);
}


void VKEngine::createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vk::ImageView attachments[] = {
			swapChainImageViews[i]
		};

		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
	}
}


void VKEngine::createCommandPool() {
	//    QueueFamilyIndices queueFamilyIndices = queueFamilyIndices; //findQueueFamilies(physicalDevice);

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

	commandPool = device.createCommandPool(poolInfo);
}

void VKEngine::createSyncObjects() {
	vk::SemaphoreCreateInfo semaphoreInfo;

	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {

		imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
		renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
		inFlightFences[i] = device.createFence(fenceInfo);

		computeFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
		computeInFlightFences[i] = device.createFence(fenceInfo);
	}
}



void VKEngine::waitForCompute() {
	ZoneScoped;
	// Compute submission   

	device.waitForFences(computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	device.resetFences(computeInFlightFences[currentFrame]);
}

uint32_t VKEngine::waitForSwapchain(WindowSetting* newSettings) {
	ZoneScoped;

	device.waitForFences(inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	device.resetFences(inFlightFences[currentFrame]);

	uint32_t imageIndex;
	auto res = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE);
	imageIndex = res.value;
	//vk::Result result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	bool settingsChanged = newSettings != nullptr;
	if (settingsChanged) {
		updateWindow(*newSettings);
	}

	if (res.result == vk::Result::eErrorOutOfDateKHR) {
		recreateSwapChain(lastUsedSwapChainSetting);
		return -1;
	}
	else {
		assert(res.result == vk::Result::eSuccess || res.result == vk::Result::eSuboptimalKHR);
	}

	return imageIndex;
}

void VKEngine::beginRenderpass(uint32_t imageIndex, vk::CommandBuffer cmdBuffer, glm::vec4 _clearColor) {
	vk::RenderPassBeginInfo renderPassInfo{};
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
	renderPassInfo.renderArea.extent = swapChainExtent;

	vk::ClearValue clearColor = vk::ClearColorValue(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;
	
	cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

vk::CommandBuffer VKEngine::getNextComputeCommandBuffer() {
	ZoneScoped;

	computeCommandBuffers[currentFrame].reset();

	vk::CommandBufferBeginInfo beginInfo;
	computeCommandBuffers[currentFrame].begin(beginInfo);

	return computeCommandBuffers[currentFrame];
}

vk::CommandBuffer VKEngine::getNextCommandBuffer() {
	ZoneScoped;

	commandBuffers[currentFrame].reset();

	vk::CommandBufferBeginInfo beginInfo;
	commandBuffers[currentFrame].begin(beginInfo);

	return commandBuffers[currentFrame];
}

void VKEngine::submitCompute() {
	ZoneScoped;

	TracyVkCollect(tracyComputeContexts[currentFrame], computeCommandBuffers[currentFrame]);

	computeCommandBuffers[currentFrame].end();

	vk::SubmitInfo submitInfo;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &computeCommandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

	computeQueue.submit(submitInfo, computeInFlightFences[currentFrame]);
}

void VKEngine::endCommandBuffer(vk::CommandBuffer commandBuffer) {
	ZoneScoped;
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

bool VKEngine::submitAndPresent(uint32_t imageIndex) {
	ZoneScoped;

	bool swapChainRecreated = false;

	TracyVkCollect(tracyGraphicsContexts[currentFrame], commandBuffers[currentFrame]);

	vk::SubmitInfo submitInfo;

	vk::Semaphore waitSemaphores[] = { computeFinishedSemaphores[currentFrame], imageAvailableSemaphores[currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	{
		ZoneScopedN("submit gfx queue");
		graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);
	}


	vk::PresentInfoKHR presentInfo;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	vk::SwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vk::Result result;
	{
		ZoneScopedN("present gfx");
		result = presentQueue.presentKHR(&presentInfo);
	}


	if(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
		framebufferResized = false;
		swapChainRecreated = true;
		recreateSwapChain(lastUsedSwapChainSetting);
	}
	else if (result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
	return swapChainRecreated;
}


void VKEngine::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::DeviceSize destinationOffset) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::BufferCopy copyRegion;
	copyRegion.size = size;
	copyRegion.dstOffset = destinationOffset;

	commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void VKEngine::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaAllocationCreateFlags flags, vk::Buffer& buffer, VmaAllocation& allocation, bool preferDevice) {

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = preferDevice ? VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE : VMA_MEMORY_USAGE_AUTO;
	if (flags != 0) {
		allocInfo.flags = flags;
	}

	vmaCreateBuffer(allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocInfo, reinterpret_cast<VkBuffer*>(&buffer), &allocation, nullptr);
}

constexpr int pipelineCount = 4; // temp

void VKEngine::createDescriptorPool() {

	vector< vk::DescriptorPoolSize> poolSizes{
		{ vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(FRAMES_IN_FLIGHT) * 10 * 2 + 2 },
		{ vk::DescriptorType::eUniformBuffer, 50 },
		{ vk::DescriptorType::eUniformBufferDynamic, 10 + 2 },
		{ vk::DescriptorType::eStorageBuffer, 30 }
	};

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT) * pipelineCount + 50;
	//  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	descriptorPool = device.createDescriptorPool(poolInfo);
}


// move to setup
void VKEngine::createTextureSamplers() {
	vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

	// linear sampler
	{
		vk::SamplerCreateInfo samplerInfo{};
		samplerInfo.magFilter = vk::Filter::eLinear;
		samplerInfo.minFilter = vk::Filter::eLinear;
		samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
		samplerInfo.anisotropyEnable = VK_TRUE;  
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		samplerInfo.unnormalizedCoordinates = VK_FALSE; 
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = vk::CompareOp::eAlways;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
		samplerInfo.minLod = 0.0f;  
		samplerInfo.maxLod = 1.0f; 

		textureSampler_linear = device.createSampler(samplerInfo);
	}
	// nearest sampler
	{
		vk::SamplerCreateInfo samplerInfo{};
		samplerInfo.magFilter = vk::Filter::eNearest;
		samplerInfo.minFilter = vk::Filter::eNearest;
		samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = vk::CompareOp::eAlways;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;

		textureSampler_nearest = device.createSampler(samplerInfo);
	}
}


vk::ImageView VKEngine::createImageView(vk::Image image, vk::Format format) {
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	vk::ImageView imageView = device.createImageView(viewInfo);

	return imageView;
}

void VKEngine::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier{};
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = vk::AccessFlags{}; 
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	commandBuffer.pipelineBarrier(
		sourceStage, destinationStage,
		vk::DependencyFlags{},
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void VKEngine::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = vk::Offset3D { 0, 0, 0 };
	region.imageExtent = vk::Extent3D {
		width,
		height,
		1
	};

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

void VKEngine::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& imageAllocation) {
	vk::ImageCreateInfo imageInfo{};
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;


	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	vmaCreateImage(allocator, reinterpret_cast<VkImageCreateInfo*>(&imageInfo), &allocInfo, reinterpret_cast<VkImage*>(&image), &imageAllocation, nullptr);
}

vk::CommandBuffer VKEngine::beginSingleTimeCommands() {
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void VKEngine::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
	
	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	graphicsQueue.submit(submitInfo);
	graphicsQueue.waitIdle();

	device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}



// keep these two functions in parity

Texture VKEngine::genTexture(int w, int h, std::vector<uint8_t>& pixels, FilterMode filterMode) {

	Texture tex;

	tex.resolutionX = w;
	tex.resolutionY = h;
	vk::DeviceSize imageSize = w * h * 4;

	vk::Buffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

	void* data;
	vmaMapMemory(allocator, stagingBufferAllocation, &data);
	memcpy(data, pixels.data(), static_cast<size_t>(imageSize));
	int tetset = 0;
	vmaUnmapMemory(allocator, stagingBufferAllocation);


	createImage(tex.resolutionX, tex.resolutionY, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, tex.textureImage, tex.textureImageAllocation);

	transitionImageLayout(tex.textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	copyBufferToImage(stagingBuffer, tex.textureImage, static_cast<uint32_t>(tex.resolutionX), static_cast<uint32_t>(tex.resolutionY));
	transitionImageLayout(tex.textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	device.destroyBuffer(stagingBuffer);
	vmaFreeMemory(allocator, stagingBufferAllocation);

	tex.imageView = createImageView(tex.textureImage, vk::Format::eR8G8B8A8Srgb);

	if (filterMode == FilterMode::Linear)
		tex.sampler = textureSampler_linear;
	else
		tex.sampler = textureSampler_nearest;

	return tex;

}

void VKEngine::genTexture(unsigned char* pixels, vk::DeviceSize imageSize, FilterMode filterMode, Texture& tex) {
	{
		vk::Buffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;
		createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(allocator, stagingBufferAllocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		int tetset = 0;
		vmaUnmapMemory(allocator, stagingBufferAllocation);

		stbi_image_free(pixels);

		createImage(tex.resolutionX, tex.resolutionY, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, tex.textureImage, tex.textureImageAllocation);

		transitionImageLayout(tex.textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		copyBufferToImage(stagingBuffer, tex.textureImage, static_cast<uint32_t>(tex.resolutionX), static_cast<uint32_t>(tex.resolutionY));
		transitionImageLayout(tex.textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vmaFreeMemory(allocator, stagingBufferAllocation);
	}

	{
		tex.imageView = createImageView(tex.textureImage, vk::Format::eR8G8B8A8Srgb);
	}

	{
		if (filterMode == FilterMode::Linear)
			tex.sampler = textureSampler_linear;
		else
			tex.sampler = textureSampler_nearest;
	}
}

Texture VKEngine::genTexture(const uint8_t* imageFileData, int dataLength, FilterMode filterMode) {
	Texture tex;
	{
		int texChannels;
		stbi_uc* pixels = stbi_load_from_memory(imageFileData, dataLength, &tex.resolutionX, &tex.resolutionY, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = tex.resolutionX * tex.resolutionY * 4;

		genTexture(pixels, imageSize, filterMode, tex);
	}
	return tex;
}

Texture VKEngine::genTexture(string imagePath, FilterMode filterMode) {

	Texture tex;


	{
		int texChannels;
		stbi_uc* pixels = stbi_load(imagePath.c_str(), &tex.resolutionX, &tex.resolutionY, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = tex.resolutionX * tex.resolutionY * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		genTexture(pixels, imageSize, filterMode, tex);
	}


	//	vk::Buffer stagingBuffer;
	//	VmaAllocation stagingBufferAllocation;
	//	auto mytemp = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	//	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

	//	void* data;
	//	vmaMapMemory(allocator, stagingBufferAllocation, &data);
	//	memcpy(data, pixels, static_cast<size_t>(imageSize));
	//	int tetset = 0;
	//	vmaUnmapMemory(allocator, stagingBufferAllocation);

	//	stbi_image_free(pixels);

	//	createImage(tex.resolutionX, tex.resolutionY, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.textureImage, tex.textureImageAllocation);

	//	transitionImageLayout(tex.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	//	copyBufferToImage(stagingBuffer, tex.textureImage, static_cast<uint32_t>(tex.resolutionX), static_cast<uint32_t>(tex.resolutionY));
	//	transitionImageLayout(tex.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//	vkDestroyBuffer(device, stagingBuffer, nullptr);
	//	vmaFreeMemory(allocator, stagingBufferAllocation);
	//}

	//{
	//	tex.imageView = createImageView(tex.textureImage, VK_FORMAT_R8G8B8A8_SRGB);
	//}

	//{
	//	if (filterMode == FilterMode::Linear)
	//		tex.sampler = textureSampler_linear;
	//	else
	//		tex.sampler = textureSampler_nearest;
	//}

	return tex;
}

void VKEngine::freeTexture(Texture& texture) {
	vmaFreeMemory(allocator, texture.textureImageAllocation);
	vkDestroyImage(device, texture.textureImage, nullptr);
	vkDestroyImageView(device, texture.imageView, nullptr);
}