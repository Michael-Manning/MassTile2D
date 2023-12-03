#pragma once

#include <GLFW/glfw3.h>

#include <vector>
#include <cstdlib>
#include <optional>
#include <array>
#include <string>
#include <memory>

#include <vulkan/vulkan.hpp>
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

template<typename T = void>
struct MappedBuffer {
	vk::Buffer buffer;
	VmaAllocation allocation;
	T* bufferMapped;
	vk::DeviceSize size;
};

template<typename T = void>
struct MappedDoubleBuffer {
	std::array<vk::Buffer, FRAMES_IN_FLIGHT> buffers;
	std::array<VmaAllocation, FRAMES_IN_FLIGHT> allocations;
	std::array<T*, FRAMES_IN_FLIGHT> buffersMapped;
	vk::DeviceSize size;
};

struct VertexMeshBuffer {
	vk::Buffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	vk::Buffer indexBuffer;
	VmaAllocation indexBufferAllocation;
};


class VKEngine {
public:

	void initWindow(const WindowSetting& settings, bool visible = true);

	void initVulkan(const SwapChainSetting& setting, int subPassCount);
	void Update();
	bool shouldClose();
	void cleanup();


	void createCommandPool();

	void createDescriptorPool();
	void createCommandBuffers();
	void createSyncObjects();

	Texture genTexture(int w, int h, std::vector<uint8_t>& pixels, FilterMode filterMode = FilterMode::Linear);
	Texture genTexture(const uint8_t * imageFileData, int dataLength, FilterMode filterMode = FilterMode::Linear);
	Texture genTexture(std::string imagePath, FilterMode filterMode = FilterMode::Linear);
	void freeTexture(Texture& texture);

	void createTextureSamplers();
	void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, VmaAllocation& imageAllocation);
	vk::ImageView createImageView(vk::Image image, vk::Format format);

	void recreateSwapChain(const SwapChainSetting& setting);
	void cleanupSwapChain();


	void createFramebuffers();

	vk::CommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(vk::CommandBuffer commandBuffer);


	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::DeviceSize destinationOffset = 0);
	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaAllocationCreateFlags flags, vk::Buffer& buffer, VmaAllocation& allocation, bool preferDevice = false);


	// should learn why this has to be implimented in the header to work
	template<typename T>
	void createMappedBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, MappedDoubleBuffer<T>& buffer) {

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			createBuffer(size, usage, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, buffer.buffers[i], buffer.allocations[i]);
			vmaMapMemory(allocator, buffer.allocations[i], reinterpret_cast<void**>(&buffer.buffersMapped[i]));
		}
		buffer.size = size;
	};

	// call in sequence
	void waitForCompute();
	uint32_t waitForSwapchain(WindowSetting* newSettings = nullptr);
	vk::CommandBuffer getNextCommandBuffer(); // temporary solution which resets the command buffer every frame
	vk::CommandBuffer getNextComputeCommandBuffer();

	void endCommandBuffer(vk::CommandBuffer commandBuffer);

	void submitCompute();
	bool submitAndPresent(uint32_t imageIndex); // returns framebuffer recreation
	void beginRenderpass(uint32_t imageIndex, vk::CommandBuffer cmdBuffer, glm::vec4 clearColor);

	bool framebufferResized = false;

	GLFWwindow* window;

	uint32_t currentFrame = 0;

	VmaAllocator allocator;

	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugMessenger;
	vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
	vk::Device device;

	vk::Queue graphicsQueue;
	vk::Queue computeQueue;
	vk::Queue presentQueue;

	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;

	// different mag filters
	vk::Sampler textureSampler_nearest;
	vk::Sampler textureSampler_linear;

	std::vector<vk::ImageView> swapChainImageViews;

	vk::RenderPass renderPass;

	std::vector<vk::Framebuffer> swapChainFramebuffers;
	vk::CommandPool commandPool;

	vk::DescriptorPool descriptorPool;


	std::array<vk::CommandBuffer, FRAMES_IN_FLIGHT> commandBuffers;
	std::array<vk::CommandBuffer, FRAMES_IN_FLIGHT>  computeCommandBuffers;

	std::array<vk::Semaphore, FRAMES_IN_FLIGHT>  imageAvailableSemaphores;
	std::array<vk::Semaphore, FRAMES_IN_FLIGHT>  renderFinishedSemaphores;
	std::array<vk::Semaphore, FRAMES_IN_FLIGHT>  computeFinishedSemaphores;
	std::array<vk::Fence, FRAMES_IN_FLIGHT> inFlightFences;
	std::array<vk::Fence, FRAMES_IN_FLIGHT> computeInFlightFences;

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
	std::array<Tracyvk::Ctx, FRAMES_IN_FLIGHT> tracyComputeContexts;
	std::array<Tracyvk::Ctx, FRAMES_IN_FLIGHT> tracyGraphicsContexts;

	void initTracyContext();
#endif

	vk::DispatchLoaderDynamic dynamicDispatcher;

private:
	void genTexture(unsigned char* pixels, vk::DeviceSize imageSize, FilterMode filterMode, Texture& tex);
	void updateWindow(WindowSetting& settings); // private to enforce synchronization
	WindowMode currentWindowMode = WindowMode::Windowed;
	int glfw_initial_primary_monitor_redBits;
	int glfw_initial_primary_monitor_greenBits;
	int glfw_initial_primary_monitor_blueBits;
	int glfw_initial_primary_monitor_refreshRate;
	int glfw_initial_primary_monitor_width;
	int glfw_initial_primary_monitor_height;
	int pre_fullscren_windowSizeX;
	int pre_fullscren_windowSizeY;
	int pre_fullscren_windowPosX = 100; // set to 100 unless starting out in windowed mode
	int pre_fullscren_windowPosY = 100;

};

