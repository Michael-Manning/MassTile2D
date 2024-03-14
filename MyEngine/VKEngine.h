#pragma once

#include <GLFW/glfw3.h>

#include <vector>
#include <cstdlib>
#include <optional>
#include <array>
#include <string>
#include <memory>
#include <variant>
#include <mutex>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <tracy/TracyVulkan.hpp>

#include "texture.h"
#include "typedefs.h"
#include "Constants.h"
#include "Settings.h"
#include "IDGenerator.h"
#include "ConcurrentQueue.h"


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

struct DeviceBuffer {
	VmaAllocation allocation;
	vk::Buffer buffer;
	vk::DeviceSize size;

	std::array<vk::Buffer, FRAMES_IN_FLIGHT> GetDoubleBuffer() {
		return { buffer, buffer };
	}
};

struct VertexMeshBuffer {
	vk::Buffer vertexBuffer;
	VmaAllocation vertexBufferAllocation;
	vk::Buffer indexBuffer;
	VmaAllocation indexBufferAllocation;
};

struct DeviceContext {
	vk::Device device;

	QueueFamilyIndices queueFamilyIndices;
	vk::Queue graphicsQueue;
	vk::Queue computeQueue;
	vk::Queue presentQueue;
};

const vk::Format framebufferImageFormat = vk::Format::eR8G8B8A8Srgb;

struct DoubleFrameBufferContext {
	vk::RenderPass renderpass;
	vk::Format format;
	std::array<vk::Extent2D, FRAMES_IN_FLIGHT> extents;
	std::array<vk::Framebuffer, FRAMES_IN_FLIGHT> framebuffers;
	std::array<Texture*, FRAMES_IN_FLIGHT> textures = { nullptr, nullptr };
	std::array<bool, FRAMES_IN_FLIGHT> resizeDirtyFlags = { false, false };
	glm::vec4 clearColor;
	glm::ivec2 targetSize;

	// technically should be in resource manager instead of stored here, but this is just way more convenient
	std::array<texID, FRAMES_IN_FLIGHT> textureIDs;
};

struct AsyncQueueSubmitInfo {
	std::vector<vk::Semaphore> waitSemaphores;
	std::vector<vk::PipelineStageFlags> waitStageFlags;
	std::vector<vk::Semaphore> signalSemaphores;
	vk::CommandBuffer* cmdBuffer;
	vk::Queue* queue = nullptr;

	// optional parameters
	vk::Fence* fence = nullptr;
	std::condition_variable* submissionCV = nullptr;
	std::atomic<bool>* submissionCompleteFlag = nullptr;
};
struct AsyncPresentSubmitInfo {
	std::vector<vk::Semaphore> waitSemaphores;
	//vk::PresentInfoKHR presentInfo;
	uint32_t imageIndex;
};

class VKEngine {
public:

	VKEngine() {
		defaultThreadID = std::this_thread::get_id();
		defaultThreadContext.id = contextIDGenerator.GenerateID();
	}

	~VKEngine() {
	}

	struct ThreadContext {
		vk::Fence SingleUseCommandFence = nullptr;
		vk::CommandPool commandPool = nullptr;
		int id = 0;
	};

	void initWindow(const WindowSetting& settings, bool visible = true);

	void initVulkan(const SwapChainSetting& setting, int subPassCount);
	void GetWindowEvents();
	bool shouldClose();
	void cleanup();


	void createCommandPools();

	void createDescriptorPool();
	void createCommandBuffers();
	void createSyncObjects();
	ThreadContext GenerateThreadContext_gfx(bool transientCommands = false);
	void createRenderPass(vk::RenderPass& renderpass, vk::Format imageFormat, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout, int subpassCount = 1);

	Texture genTexture(int w, int h, std::vector<uint8_t>& pixels, FilterMode filterMode, const ThreadContext& context);
	Texture genTexture(const uint8_t * imageFileData, int dataLength, FilterMode filterMode, const ThreadContext& context);
	Texture genTexture(std::string imagePath, FilterMode filterMode, const ThreadContext& context);
	
	Texture genTexture(int w, int h, std::vector<uint8_t>& pixels, FilterMode filterMode = FilterMode::Linear) {
		assert(defaultThreadID == std::this_thread::get_id());
		return genTexture(w, h, pixels, filterMode, defaultThreadContext);
	}
	Texture genTexture(const uint8_t* imageFileData, int dataLength, FilterMode filterMode = FilterMode::Linear) {
		assert(defaultThreadID == std::this_thread::get_id());
		return genTexture(imageFileData, dataLength, filterMode, defaultThreadContext);
	}
	Texture genTexture(std::string imagePath, FilterMode filterMode = FilterMode::Linear) {
		assert(defaultThreadID == std::this_thread::get_id());
		return genTexture(imagePath, filterMode, defaultThreadContext);
	}
	
	void freeTexture(Texture& texture);

	void createTextureSamplers();
	void transitionImageLayout(vk::CommandBuffer& cmdBuffer, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void copyBufferToImage(vk::CommandBuffer& cmdBuffer, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image* image, VmaAllocation& imageAllocation);
	vk::ImageView createImageView(vk::Image image, vk::Format format);

	void recreateSwapChain(const SwapChainSetting& setting);
	void cleanupSwapChain();


	//void createFramebuffers();

	vk::CommandBuffer beginSingleTimeCommands_gfx(const ThreadContext& context);
	vk::CommandBuffer beginSingleTimeCommands_gfx() {
		assert(defaultThreadID == std::this_thread::get_id());
		return beginSingleTimeCommands_gfx(defaultThreadContext);
	}

	void endSingleTimeCommands_gfx(vk::CommandBuffer commandBuffer, const ThreadContext& context);
	void endSingleTimeCommands_gfx(vk::CommandBuffer commandBuffer) {
		assert(defaultThreadID == std::this_thread::get_id());
		endSingleTimeCommands_gfx(commandBuffer, defaultThreadContext);
	}

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

	template<typename T>
	void createMappedBuffer(vk::BufferUsageFlags usage, MappedDoubleBuffer<T>& buffer) {
		createMappedBuffer(sizeof(T), usage, buffer);
	}

	void WaitForComputeSubmission();
	void WaitForComputeCompletion();
	
	void WaitForGraphicsSubmission();
	void WaitForGraphicsCompletion();

	void WaitForFramePresentationSubmission(); 
	uint32_t WaitForSwapChainImageAvailableAndHandleWindowChanges(WindowSetting* newSettings);

	
	vk::CommandBuffer getNextGfxCommandBuffer(); // temporary solution which resets the command buffer every frame
	vk::CommandBuffer getNextComputeCommandBuffer();

	void endCommandBuffer(vk::CommandBuffer commandBuffer);

	void QueueComputeSubmission();
	void QueueGraphicsSubmission();
	void QueuePresentSubmission(uint32_t imageIndex);
	void IncrementFrameInFlight() {
		currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
	};
	


	void beginSwapchainRenderpass(uint32_t imageIndex, vk::CommandBuffer cmdBuffer, glm::vec4 clearColor);
	void beginRenderpass(DoubleFrameBufferContext* framebufferContext, vk::CommandBuffer cmdBuffer);

	void CreateDoubleFrameBuffer(glm::ivec2 size, DoubleFrameBufferContext& dfb, const ThreadContext& context, glm::vec4 clearColor, vk::Format format);
	// effectively resizes the framebuffer, modifying the vulkan objects of the specified index
	void RecreateFramebuffer(glm::ivec2 size, DoubleFrameBufferContext* dfb, int index, const ThreadContext& context);

	void insertFramebufferTransitionBarrier(vk::CommandBuffer cmdBuffer, DoubleFrameBufferContext* framebufferContext);

	bool framebufferResized = false;

	GLFWwindow* window;

	uint32_t currentFrame = 0;

	VmaAllocator allocator;

	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugMessenger;
	vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;

	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapChain;
	vk::Format swapChainImageFormat;

	vk::RenderPass swapchainRenderPass;
	std::vector<vk::Image> swapChainImages;
	vk::Extent2D swapChainExtent;

	// different mag filters
	vk::Sampler textureSampler_nearest;
	vk::Sampler textureSampler_linear;

	std::vector<vk::ImageView> swapChainImageViews;
	std::vector<vk::Framebuffer> swapChainFramebuffers;
	vk::DescriptorPool descriptorPool;

	DeviceContext devContext;

	std::array<vk::CommandBuffer, FRAMES_IN_FLIGHT> graphicsCommandBuffers;
	std::array<vk::CommandBuffer, FRAMES_IN_FLIGHT>  computeCommandBuffers;

	std::array<vk::Semaphore, FRAMES_IN_FLIGHT>  imageAvailableSemaphores;
	std::array<vk::Semaphore, FRAMES_IN_FLIGHT>  renderFinishedSemaphores;
	std::array<vk::Semaphore, FRAMES_IN_FLIGHT>  computeFinishedSemaphores;
	std::array<vk::Fence, FRAMES_IN_FLIGHT> swapchainFBRenderFinishedFences;
	std::array<vk::Fence, FRAMES_IN_FLIGHT> computeInFlightFences;

	std::mutex queueSubmitMutex;

	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain(SwapChainSetting setting);
	//void createRenderPass(int subpassCount = 1);

	SwapChainSetting lastUsedSwapChainSetting;

#ifdef TRACY_ENABLE
	std::array<TracyVkCtx, FRAMES_IN_FLIGHT> tracyComputeContexts;
	std::array<TracyVkCtx, FRAMES_IN_FLIGHT> tracyGraphicsContexts;

	void initTracyContext();
#endif

	vk::DispatchLoaderDynamic dynamicDispatcher;

	std::thread::id defaultThreadID;
	ThreadContext defaultThreadContext;

	void AddToSubmitQueue(AsyncQueueSubmitInfo info) {
		queueSubmitQueue.push(info);
	};
	void AddToSubmitQueue(AsyncPresentSubmitInfo info) {
		queueSubmitQueue.push(info);
	};

	bool ShouldRecreateSwapchain() {
		return swapchainOutOfDate.load();
	}

private:

	std::array <std::atomic<bool>, FRAMES_IN_FLIGHT> computeSubmittedFlags;
	std::array <std::atomic<bool>, FRAMES_IN_FLIGHT> graphicsSubmittedFlags;

	std::array <std::condition_variable, FRAMES_IN_FLIGHT> computeSubmitCompleteCVs;
	std::array <std::condition_variable, FRAMES_IN_FLIGHT> graphicsSubmitCompleteCVs;

	// Have a dedicated thread which only transfers data to the GPU
	std::thread queueSubmitThread;
	std::atomic<bool> queueSubmitThreadCancel = false;
	ConcurrentQueue<std::variant<AsyncQueueSubmitInfo, AsyncPresentSubmitInfo>> queueSubmitQueue;
	std::mutex presentationMutex;
	// graphics doesn't need a condition variable becauseit is gated by the presentation CV
	std::condition_variable presentationCV;
	std::atomic<bool> presentSubmitted = false;
	std::atomic<bool> swapchainOutOfDate = false;

	void queueSubmitThreadWorkFunc();
	void cancelSubmitThread();

	void createSwapchainFramebuffers();

	IDGenerator<int> contextIDGenerator;

	void genTexture(unsigned char* pixels, vk::DeviceSize imageSize, FilterMode filterMode, Texture& tex, const ThreadContext& context);
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

