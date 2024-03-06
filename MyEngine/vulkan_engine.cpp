//#define VK_USE_PLATFORM_WIN32_KHR
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>

#include "stdafx.h"

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

void VKEngine::createRenderPass(vk::RenderPass& renderpass, vk::Format imageFormat, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout, int subpassCount) {

	assert(subpassCount < 3); // not properly supporting more because of the way the subpass dependencies are set up

	vk::AttachmentDescription colorAttachment{};
	colorAttachment.format = imageFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = initialLayout;
	colorAttachment.finalLayout = finalLayout;

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

	vk::RenderPassCreateInfo renderPassInfo({});
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = subpassCount;
	renderPassInfo.pSubpasses = subpasses.data();

	if (subpassCount > 1) {
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;
	}

	renderpass = devContext.device.createRenderPass(renderPassInfo);
}


void VKEngine::createCommandPools() {

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = devContext.queueFamilyIndices.graphicsAndComputeFamily.value();

	defaultThreadContext.commandPool = devContext.device.createCommandPool(poolInfo);
}

void VKEngine::createSyncObjects() {

	vk::SemaphoreCreateInfo semaphoreInfo;
	vk::FenceCreateInfo fenceInfo;

	defaultThreadContext.SingleUseCommandFence = devContext.device.createFence(fenceInfo);

	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		imageAvailableSemaphores[i] = devContext.device.createSemaphore(semaphoreInfo);
		renderFinishedSemaphores[i] = devContext.device.createSemaphore(semaphoreInfo);
		swapchainFBRenderFinishedFences[i] = devContext.device.createFence(fenceInfo);

		computeFinishedSemaphores[i] = devContext.device.createSemaphore(semaphoreInfo);
		computeInFlightFences[i] = devContext.device.createFence(fenceInfo);
	}
}

// Eventually use with async texture creation
VKEngine::ThreadContext VKEngine::GenerateThreadContext_gfx(bool transientCommands) {

	ThreadContext context;

	vk::FenceCreateInfo fenceInfo{};

	context.SingleUseCommandFence = devContext.device.createFence(vk::FenceCreateInfo());

	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	if (transientCommands)
		poolInfo.flags |= vk::CommandPoolCreateFlagBits::eTransient;
	poolInfo.queueFamilyIndex = devContext.queueFamilyIndices.graphicsAndComputeFamily.value();

	context.commandPool = devContext.device.createCommandPool(poolInfo);

	context.id = contextIDGenerator.GenerateID();

	return context;
}

void VKEngine::WaitForComputeSubmission() {
	ZoneScopedNC("Wait for compute submission", 0x0000FF);

	std::unique_lock<std::mutex> lock(queueSubmitMutex);
	if (!computeSubmittedFlags[currentFrame]) {
		computeSubmitCompleteCVs[currentFrame].wait(lock, [this] {return computeSubmittedFlags[currentFrame].load(); });
	}
	computeSubmittedFlags[currentFrame].store(false);
}
void VKEngine::WaitForComputeCompletion() {
	ZoneScopedNC("Wait for compute completion", 0x0000FF);

	devContext.device.waitForFences(1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	devContext.device.resetFences(1, &computeInFlightFences[currentFrame]);
}

void VKEngine::WaitForGraphicsSubmission() {
	ZoneScopedNC("Wait for graphics submission", 0x0000FF);

	std::unique_lock<std::mutex> lock(queueSubmitMutex);
	if (!graphicsSubmittedFlags[currentFrame]) {
		graphicsSubmitCompleteCVs[currentFrame].wait(lock, [this] {return graphicsSubmittedFlags[currentFrame].load(); });
	}
	graphicsSubmittedFlags[currentFrame].store(false);
}

void VKEngine::WaitForGraphicsCompletion() {
	ZoneScopedNC("Wait for graphics completion", 0x0000FF);
	devContext.device.waitForFences(1, &swapchainFBRenderFinishedFences[currentFrame], VK_TRUE, UINT64_MAX);
	devContext.device.resetFences(1, &swapchainFBRenderFinishedFences[currentFrame]);
}

void VKEngine::WaitForFramePresentationSubmission() {
	ZoneScopedNC("Wait for presentation submission", 0x0000FF);
	std::unique_lock<std::mutex> lock(presentationMutex);
	if (!presentSubmitted) {
		presentationCV.wait(lock, [this] {return presentSubmitted.load(); });
	}
	presentSubmitted.store(false);
}

uint32_t VKEngine::WaitForSwapChainImageAvailableAndHandleWindowChanges(WindowSetting* newSettings) {

	ZoneScopedNC("Wait for image available", 0x0000FF);

	uint32_t imageIndex;
	auto res = devContext.device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE);
	imageIndex = res.value;

	bool settingsChanged = newSettings != nullptr;
	if (settingsChanged) {
		updateWindow(*newSettings);
	}

	if (res.result == vk::Result::eErrorOutOfDateKHR) {
		// it's apparently possible for the swapchain to be out of date here, but I've never encountered it, so it's not hooked up
		// to the logic for recreating the swapchain. Should look more into this. I suspect (for no reason) that as lonng as the 
		// error is handled and swapchain recreated before presenting the previous frame that there shouldn't be an error next
		// time we wait on the swapchain be requresting the next image
		assert(false);
	}
	else {
		assert(res.result == vk::Result::eSuccess || res.result == vk::Result::eSuboptimalKHR);
	}

	return imageIndex;
}

void VKEngine::beginSwapchainRenderpass(uint32_t imageIndex, vk::CommandBuffer cmdBuffer, glm::vec4 _clearColor) {
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = swapchainRenderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
	renderPassInfo.renderArea.extent = swapChainExtent;

	vk::ClearValue clearColor = vk::ClearColorValue(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void VKEngine::beginRenderpass(DoubleFrameBufferContext* framebufferContext, vk::CommandBuffer cmdBuffer) {
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = framebufferContext->renderpass;
	renderPassInfo.framebuffer = framebufferContext->framebuffers[currentFrame];
	renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
	renderPassInfo.renderArea.extent = framebufferContext->extents[currentFrame];

	vk::ClearValue clearColor = vk::ClearColorValue(
		framebufferContext->clearColor.r,
		framebufferContext->clearColor.g,
		framebufferContext->clearColor.b,
		framebufferContext->clearColor.a
	);
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

vk::CommandBuffer VKEngine::getNextGfxCommandBuffer() {
	ZoneScoped;
	graphicsCommandBuffers[currentFrame].reset();
	graphicsCommandBuffers[currentFrame].begin(vk::CommandBufferBeginInfo({}));
	return graphicsCommandBuffers[currentFrame];
}

void VKEngine::QueueComputeSubmission() {
	ZoneScoped;

	AsyncQueueSubmitInfo qsi;

	qsi.cmdBuffer = &computeCommandBuffers[currentFrame];

	qsi.signalSemaphores.push_back(computeFinishedSemaphores[currentFrame]);

	qsi.fence = &computeInFlightFences[currentFrame];
	qsi.queue = &devContext.computeQueue;

	qsi.submissionCompleteFlag = &computeSubmittedFlags[currentFrame];
	qsi.submissionCV = &computeSubmitCompleteCVs[currentFrame];

	AddToSubmitQueue(qsi);

}

void VKEngine::endCommandBuffer(vk::CommandBuffer commandBuffer) {
	ZoneScoped;
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}


void VKEngine::QueueGraphicsSubmission() {
	AsyncQueueSubmitInfo qsi;

	qsi.waitSemaphores.push_back(computeFinishedSemaphores[currentFrame]);
	qsi.waitStageFlags.push_back(vk::PipelineStageFlagBits::eVertexInput);

	qsi.waitSemaphores.push_back(imageAvailableSemaphores[currentFrame]);
	qsi.waitStageFlags.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);

	qsi.cmdBuffer = &graphicsCommandBuffers[currentFrame];

	qsi.signalSemaphores.push_back(renderFinishedSemaphores[currentFrame]);

	qsi.fence = &swapchainFBRenderFinishedFences[currentFrame];
	qsi.queue = &devContext.graphicsQueue;

	qsi.submissionCompleteFlag = &graphicsSubmittedFlags[currentFrame];
	qsi.submissionCV = &graphicsSubmitCompleteCVs[currentFrame];

	AddToSubmitQueue(qsi);
}

void VKEngine::QueuePresentSubmission(uint32_t imageIndex) {
	AsyncPresentSubmitInfo psi;

	psi.waitSemaphores.push_back(renderFinishedSemaphores[currentFrame]);
	psi.imageIndex = imageIndex;

	psi.imageIndex = imageIndex;

	AddToSubmitQueue(psi);
}

#if false
bool VKEngine::submitAndPresent(uint32_t imageIndex) {
	ZoneScoped;

	bool swapChainRecreated = false;

	//TracyVkCollect(tracyGraphicsContexts[currentFrame], graphicsCommandBuffers[currentFrame]);


	vk::SubmitInfo submitInfo;

	vk::Semaphore waitSemaphores[] = { computeFinishedSemaphores[currentFrame], imageAvailableSemaphores[currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eVertexInput, vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &graphicsCommandBuffers[currentFrame];

	vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	{
		ZoneScopedN("submit gfx queue");
		devContext.graphicsQueue.submit(submitInfo, swapchainFBRenderFinishedFences[currentFrame]);
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
		result = devContext.presentQueue.presentKHR(&presentInfo);
	}


	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
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
#endif



void VKEngine::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::DeviceSize destinationOffset) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands_gfx();

	vk::BufferCopy copyRegion;
	copyRegion.size = size;
	copyRegion.dstOffset = destinationOffset;

	commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands_gfx(commandBuffer);
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
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;

	descriptorPool = devContext.device.createDescriptorPool(poolInfo);
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

	vk::ImageView imageView = devContext.device.createImageView(viewInfo);

	return imageView;
}

void VKEngine::transitionImageLayout(vk::CommandBuffer& cmdBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {

	vk::ImageMemoryBarrier barrier;
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
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
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
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral) {
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eAllCommands;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eGeneral) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eAllCommands;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	cmdBuffer.pipelineBarrier(
		sourceStage,
		destinationStage,
		static_cast<vk::DependencyFlags>(0),
		0, nullptr,
		0, nullptr,
		1, &barrier);
}

void VKEngine::copyBufferToImage(vk::CommandBuffer& cmdBuffer, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {

	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = vk::Offset3D(0, 0, 0);
	region.imageExtent = vk::Extent3D(
		width,
		height,
		1
	);

	cmdBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
}

void VKEngine::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image* image, VmaAllocation& imageAllocation) {
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
	vmaCreateImage(allocator, reinterpret_cast<VkImageCreateInfo*>(&imageInfo), &allocInfo, reinterpret_cast<VkImage*>(image), &imageAllocation, nullptr);
}

vk::CommandBuffer VKEngine::beginSingleTimeCommands_gfx(const ThreadContext& context) {
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = context.commandPool;
	allocInfo.commandBufferCount = 1;

	vk::CommandBuffer commandBuffer = devContext.device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void VKEngine::endSingleTimeCommands_gfx(vk::CommandBuffer commandBuffer, const ThreadContext& context) {
	commandBuffer.end();
	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	{
		std::lock_guard<std::mutex> lock(queueSubmitMutex);
		devContext.graphicsQueue.submit(1, &submitInfo, context.SingleUseCommandFence);
		std::ignore = devContext.device.waitForFences(1, &context.SingleUseCommandFence, VK_TRUE, UINT64_MAX);
		std::ignore = devContext.device.resetFences(1, &context.SingleUseCommandFence);
	}

	devContext.device.freeCommandBuffers(context.commandPool, 1, &commandBuffer);
}


// keep these two functions in parity

Texture VKEngine::genTexture(int w, int h, std::vector<uint8_t>& pixels, FilterMode filterMode, const ThreadContext& context) {

	Texture tex = {};

	tex.resolutionX = w;
	tex.resolutionY = h;
	VkDeviceSize imageSize = w * h * 4;

	vk::Buffer stagingBuffer;
	VmaAllocation stagingBufferAllocation = nullptr;

	const bool hasData = pixels.size() > 0;

	if (hasData) {
		createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);
		void* data;
		vmaMapMemory(allocator, stagingBufferAllocation, &data);
		memcpy(data, pixels.data(), static_cast<size_t>(imageSize));
		int tetset = 0;
		vmaUnmapMemory(allocator, stagingBufferAllocation);
	}

	vk::Format format = vk::Format::eR8G8B8A8Srgb;
	vk::ImageTiling imageTiling = vk::ImageTiling::eOptimal;
	vk::ImageUsageFlags imageUsage;
	imageUsage |= vk::ImageUsageFlagBits::eSampled;
	if (hasData)
		imageUsage |= vk::ImageUsageFlagBits::eTransferDst;

	createImage(
		tex.resolutionX, tex.resolutionY,
		format,
		imageTiling,
		imageUsage,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		&tex.textureImage,
		tex.textureImageAllocation);

	vk::CommandBuffer cmdBuffer = beginSingleTimeCommands_gfx(context);
	if (pixels.size() == 0) {
		transitionImageLayout(cmdBuffer, tex.textureImage, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);
	}
	else if (pixels.size() >= w * h) {
		transitionImageLayout(cmdBuffer, tex.textureImage, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		copyBufferToImage(cmdBuffer, stagingBuffer, tex.textureImage, static_cast<uint32_t>(tex.resolutionX), static_cast<uint32_t>(tex.resolutionY));
		transitionImageLayout(cmdBuffer, tex.textureImage, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	}
	else {
		// not enough data to populate texture
		assert(false);
	}

	endSingleTimeCommands_gfx(cmdBuffer, context);

	if (hasData) {
		vkDestroyBuffer(devContext.device, stagingBuffer, nullptr);
		vmaFreeMemory(allocator, stagingBufferAllocation);
	}

	tex.imageView = createImageView(tex.textureImage, format);

	if (filterMode == FilterMode::Linear)
		tex.sampler = textureSampler_linear;
	else
		tex.sampler = textureSampler_nearest;

	return tex;
}

void VKEngine::genTexture(unsigned char* pixels, vk::DeviceSize imageSize, FilterMode filterMode, Texture& tex, const ThreadContext& context) {


	vk::Buffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

	void* data;
	vmaMapMemory(allocator, stagingBufferAllocation, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(allocator, stagingBufferAllocation);

	stbi_image_free(pixels);

	vk::Format format = vk::Format::eR8G8B8A8Srgb;
	vk::ImageTiling imageTiling = vk::ImageTiling::eOptimal;
	vk::ImageUsageFlags imageUsage;
	imageUsage |= vk::ImageUsageFlagBits::eSampled;
	imageUsage |= vk::ImageUsageFlagBits::eTransferDst;

	createImage(
		tex.resolutionX, tex.resolutionY,
		format,
		imageTiling,
		imageUsage,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		&tex.textureImage,
		tex.textureImageAllocation);

	vk::CommandBuffer cmdBuffer = beginSingleTimeCommands_gfx(context);
	transitionImageLayout(cmdBuffer, tex.textureImage, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	copyBufferToImage(cmdBuffer, stagingBuffer, tex.textureImage, static_cast<uint32_t>(tex.resolutionX), static_cast<uint32_t>(tex.resolutionY));
	transitionImageLayout(cmdBuffer, tex.textureImage, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	endSingleTimeCommands_gfx(cmdBuffer, context);

	vkDestroyBuffer(devContext.device, stagingBuffer, nullptr);
	vmaFreeMemory(allocator, stagingBufferAllocation);


	tex.imageView = createImageView(tex.textureImage, format);

	{
		if (filterMode == FilterMode::Linear)
			tex.sampler = textureSampler_linear;
		else
			tex.sampler = textureSampler_nearest;
	}
}

Texture VKEngine::genTexture(const uint8_t* imageFileData, int dataLength, FilterMode filterMode, const ThreadContext& context) {
	Texture tex = {};
	{
		int texChannels;
		stbi_uc* pixels = stbi_load_from_memory(imageFileData, dataLength, &tex.resolutionX, &tex.resolutionY, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = tex.resolutionX * tex.resolutionY * 4;

		genTexture(pixels, imageSize, filterMode, tex, context);
	}
	return tex;
}


Texture VKEngine::genTexture(string imagePath, FilterMode filterMode, const ThreadContext& context) {

	Texture tex = {};

	assert(std::filesystem::exists(std::filesystem::path(imagePath)));

	{
		int texChannels;
		stbi_uc* pixels = stbi_load(imagePath.c_str(), &tex.resolutionX, &tex.resolutionY, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = tex.resolutionX * tex.resolutionY * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		genTexture(pixels, imageSize, filterMode, tex, context);
	}

	return tex;
}

void VKEngine::freeTexture(Texture& texture) {
	vmaFreeMemory(allocator, texture.textureImageAllocation);
	vkDestroyImage(devContext.device, texture.textureImage, nullptr);
	vkDestroyImageView(devContext.device, texture.imageView, nullptr);
}

// Does not allocate texture pointers! Must provide valid texture pointers
void VKEngine::CreateDoubleFrameBuffer(glm::ivec2 size, DoubleFrameBufferContext& dfb, const ThreadContext& context, vec4 clearColor, vk::Format format) {

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		assert(dfb.textures[i] != nullptr);
	}

	dfb.clearColor = clearColor;
	dfb.targetSize = size;
	dfb.format = format;

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		dfb.extents[i] = VkExtent2D{
			static_cast<uint32_t>(size.x),
			static_cast<uint32_t>(size.y)
		};
	}

	// images
	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		createImage(size.x, size.y,
			format,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eSampled |
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			&dfb.textures[i]->textureImage,
			dfb.textures[i]->textureImageAllocation);

		dfb.textures[i]->resolutionX = size.x;
		dfb.textures[i]->resolutionY = size.y;

		vk::CommandBuffer cmdBuffer = beginSingleTimeCommands_gfx(context);
		transitionImageLayout(cmdBuffer, dfb.textures[i]->textureImage, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
		endSingleTimeCommands_gfx(cmdBuffer, context);
	}

	// image views
	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		dfb.textures[i]->imageView = createImageView(dfb.textures[i]->textureImage, format);
	}

	// samplers
	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		dfb.textures[i]->sampler = textureSampler_linear;
	}

	// imgui textures
	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		dfb.textures[i]->imTexture = ImGui_ImplVulkan_AddTexture(dfb.textures[i]->sampler, dfb.textures[i]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	// render pass
	createRenderPass(dfb.renderpass, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal, 1);

	// framebuffers
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		vk::ImageView attachments[] = {
			dfb.textures[i]->imageView
		};

		vk::FramebufferCreateInfo framebufferInfo({});
		//framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = dfb.renderpass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = size.x;
		framebufferInfo.height = size.y;
		framebufferInfo.layers = 1;

		dfb.framebuffers[i] = devContext.device.createFramebuffer(framebufferInfo);
	}
}

void VKEngine::RecreateFramebuffer(glm::ivec2 size, DoubleFrameBufferContext* dfb, int index, const ThreadContext& context) {

	devContext.device.destroyFramebuffer(dfb->framebuffers[index]);

	assert(size.x > 10 && size.y > 10); // sanity

	dfb->extents[index] = VkExtent2D{
		static_cast<uint32_t>(size.x),
		static_cast<uint32_t>(size.y)
	};

	freeTexture(*dfb->textures[index]);

	// image
	createImage(size.x, size.y,
		dfb->format,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		&dfb->textures[index]->textureImage,
		dfb->textures[index]->textureImageAllocation);

	dfb->textures[index]->resolutionX = size.x;
	dfb->textures[index]->resolutionY = size.y;

	vk::CommandBuffer cmdBuffer = beginSingleTimeCommands_gfx(context);
	transitionImageLayout(cmdBuffer, dfb->textures[index]->textureImage, dfb->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
	endSingleTimeCommands_gfx(cmdBuffer, context);

	// image view
	dfb->textures[index]->imageView = createImageView(dfb->textures[index]->textureImage, dfb->format);

	// sampler
	dfb->textures[index]->sampler = textureSampler_linear;

	// Update the ImGui Descriptor Set
	{
		VkDescriptorImageInfo desc_image[1] = {};
		desc_image[0].sampler = dfb->textures[index]->sampler;
		desc_image[0].imageView = dfb->textures[index]->imageView;
		desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet write_desc[1] = {};
		write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_desc[0].dstSet = dfb->textures[index]->imTexture.value();
		write_desc[0].descriptorCount = 1;
		write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_desc[0].pImageInfo = desc_image;
		vkUpdateDescriptorSets(devContext.device, 1, write_desc, 0, nullptr);
	}

	// framebuffers
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		vk::ImageView attachments[] = {
			dfb->textures[index]->imageView
		};

		vk::FramebufferCreateInfo framebufferInfo({});
		framebufferInfo.renderPass = dfb->renderpass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = size.x;
		framebufferInfo.height = size.y;
		framebufferInfo.layers = 1;

		dfb->framebuffers[index] = devContext.device.createFramebuffer(framebufferInfo);
	}
}

void VKEngine::insertFramebufferTransitionBarrier(vk::CommandBuffer cmdBuffer, DoubleFrameBufferContext* framebufferContext) {

	// doesn't actually cause a transition, just used for synchronization purposes to ensure the transition specified by
	// the render pass has actually completed before the fragment shader tries to access it.

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = framebufferContext->textures[currentFrame]->textureImage;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; // Writes in the first render pass.
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead; // Reads in the second render pass.

	cmdBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader,
		static_cast<vk::DependencyFlags>(0),
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void VKEngine::cancelSubmitThread() {
	queueSubmitThreadCancel.store(true);
	queueSubmitQueue.cancel_wait();
	queueSubmitThread.join();
}

void VKEngine::queueSubmitThreadWorkFunc() {

	// basically just pops either a QueueSubmitInfo or PresentSubmitInfo off the queue and proccess them accordingly

	while (queueSubmitThreadCancel.load(std::memory_order::relaxed) == false) {
		std::variant<AsyncQueueSubmitInfo, AsyncPresentSubmitInfo> info;
		if (queueSubmitQueue.size() && queueSubmitQueue.wait_and_pop(info)) {


			if (std::holds_alternative<AsyncQueueSubmitInfo>(info)) {
				AsyncQueueSubmitInfo qsi = std::get<AsyncQueueSubmitInfo>(info);

				{
					ZoneScopedN("submit queue");
					std::lock_guard<std::mutex> lock(queueSubmitMutex);

					assert(qsi.queue != nullptr);
					vk::SubmitInfo info;

					info.waitSemaphoreCount = qsi.waitSemaphores.size();
					info.pWaitSemaphores = qsi.waitSemaphores.data();
					info.pWaitDstStageMask = qsi.waitStageFlags.data();

					info.commandBufferCount = 1;
					info.pCommandBuffers = qsi.cmdBuffer;

					info.signalSemaphoreCount = qsi.signalSemaphores.size();
					info.pSignalSemaphores = qsi.signalSemaphores.data();

					if (qsi.fence == nullptr)
						qsi.queue->submit(info);
					else
						qsi.queue->submit(info, *qsi.fence);

					if (qsi.submissionCompleteFlag != nullptr)
						qsi.submissionCompleteFlag->store(true);
					if (qsi.submissionCV != nullptr)
						qsi.submissionCV->notify_one();
				}

				continue;
			}
			if (std::holds_alternative<AsyncPresentSubmitInfo>(info)) {

				std::unique_lock<std::mutex> lock(presentationMutex);

				AsyncPresentSubmitInfo psi = std::get<AsyncPresentSubmitInfo>(info);

				vk::Result result;

				{
					std::lock_guard<std::mutex> lock(queueSubmitMutex);
					ZoneScopedN("present gfx");

					vk::PresentInfoKHR info;

					info.waitSemaphoreCount = psi.waitSemaphores.size();
					info.pWaitSemaphores = psi.waitSemaphores.data();

					info.swapchainCount = 1;
					info.pSwapchains = &swapChain;

					info.pImageIndices = &psi.imageIndex;

					// use function overload which does not throw exceptions
					result = devContext.presentQueue.presentKHR(&info);
				}

				if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
					// window was resized or moved to a different monitor
					swapchainOutOfDate.store(true);
				}
				else if (result != vk::Result::eSuccess) {
					throw std::runtime_error("failed to present swap chain image!");
				}

				presentSubmitted.store(true);
				presentationCV.notify_one();

				continue;
			}

			assert(false);
		}
	}
}