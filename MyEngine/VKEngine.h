#pragma once

#include <GLFW/glfw3.h>

#include <vector>
#include <cstdlib>
#include <optional>
#include <array>
#include <string>
#include <memory>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vk_mem_alloc.h>
#include <tracy/TracyVulkan.hpp>

#include "texture.h"
#include "typedefs.h"
#include "Constants.h"
#include "Settings.h"


struct QueueFamilyIndices {
	//std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> graphicsAndComputeFamily;

	bool isComplete() {
		return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
	}
};

struct MappedBuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
	void* bufferMapped;
	VkDeviceSize size;
};

struct MappedDoubleBuffer {
	std::array<VkBuffer, FRAMES_IN_FLIGHT> buffers;
	std::array<VmaAllocation, FRAMES_IN_FLIGHT> allocations;
	std::array<void*, FRAMES_IN_FLIGHT> buffersMapped;
	VkDeviceSize size;
};

struct VertexMeshBuffer {
	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	VkBuffer indexBuffer;
	VmaAllocation indexBufferAllocation;
};

class VKEngine {
public:

	void initWindow(int width, int height, std::string name, bool visible = true);
	void initVulkan(SwapChainSetting setting, int subPassCount);
	void Update();
	bool shouldClose();
	void cleanup();


	void createCommandPool();

	void createDescriptorPool();
	void createCommandBuffers();
	void createSyncObjects();

	Texture genTexture(int w, int h, std::vector<uint8_t>& pixels, FilterMode filterMode = FilterMode::Linear);
	Texture genTexture(std::string imagePath, FilterMode filterMode = FilterMode::Linear);

	void createTextureSamplers();
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& imageAllocation);
	VkImageView createImageView(VkImage image, VkFormat format);

	void recreateSwapChain(SwapChainSetting setting);
	void cleanupSwapChain();


	void createFramebuffers();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);


	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize destinationOffset = 0);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, VkBuffer& buffer, VmaAllocation& allocation, bool preferDevice = false);
	void createMappedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, MappedDoubleBuffer& buffer);

	// call in sequence
	void waitForCompute();
	uint32_t waitForSwapchain();
	VkCommandBuffer getNextCommandBuffer(); // temporary solution which resets the command buffer every frame
	VkCommandBuffer getNextComputeCommandBuffer();

	void endCommandBuffer(VkCommandBuffer commandBuffer);

	void submitCompute();
	bool submitAndPresent(uint32_t imageIndex); // returns framebuffer recreation
	void beginRenderpass(uint32_t imageIndex, VkCommandBuffer cmdBuffer, glm::vec4 clearColor);

	bool framebufferResized = false;
	//private:
	GLFWwindow* window;

	uint32_t currentFrame = 0;

	int winW;
	int winH;

	VmaAllocator allocator;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue presentQueue;

	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// different mag filters
	VkSampler textureSampler_nearest;
	VkSampler textureSampler_linear;

	std::vector<VkImageView> swapChainImageViews;

	VkRenderPass renderPass;

	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;

	VkDescriptorPool descriptorPool;


	std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> commandBuffers;
	std::array<VkCommandBuffer, FRAMES_IN_FLIGHT>  computeCommandBuffers;

	std::array<VkSemaphore, FRAMES_IN_FLIGHT>  imageAvailableSemaphores;
	std::array<VkSemaphore, FRAMES_IN_FLIGHT>  renderFinishedSemaphores;
	std::array<VkSemaphore, FRAMES_IN_FLIGHT>  computeFinishedSemaphores;
	std::array<VkFence, FRAMES_IN_FLIGHT> inFlightFences;
	std::array<VkFence, FRAMES_IN_FLIGHT> computeInFlightFences;

	QueueFamilyIndices queueFamilyIndices;

	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain(SwapChainSetting setting);
	void createImageViews();
	//void createRenderPass();
	void createRenderPass(int subpassCount = 1);

	SwapChainSetting lastUsedSwapChainSetting;

#ifdef TRACY_ENABLE
	std::array<TracyVkCtx, FRAMES_IN_FLIGHT> tracyComputeContexts;
	std::array<TracyVkCtx, FRAMES_IN_FLIGHT> tracyGraphicsContexts;

	void initTracyContext();
#endif
};

