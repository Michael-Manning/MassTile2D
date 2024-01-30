#include "stdafx.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

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
#include <regex>
#include <string>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

//#define VMA_VULKAN_VERSION 1002000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1 
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "VKEngine.h"

#define ENGINE_VERSION VK_MAKE_VERSION(1, 2, 0)
#define ENGINE_API_VERSION VK_API_VERSION_1_2
#define ENGINE_NAME "MichaelEngineV1"
#define APPLICATION_NAME "MyGame" 

using namespace glm;
using namespace std;

 
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
};

const std::vector<const char*> instanceExtensions = {
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};


#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};


// private forward declarations
SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);


std::string insertBeforeMatch(const std::string& mainStr, const std::string& match) {
	std::string result = mainStr;
	size_t pos = result.find(match);

	if (pos != std::string::npos && pos > 0) {  // If match is found and it's not at the beginning
		result.insert(pos - 1, "\n\n");
	}

	return result;
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {

		cout << endl;
		string msgStr = string(pCallbackData->pMessage);
		cout << insertBeforeMatch(msgStr, "The Vulkan spec states");
		cout << endl;

	}
	else {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {

	createInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	createInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
	createInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
	createInfo.messageType |= vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral;
	createInfo.messageType |= vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	createInfo.messageType |= vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

	createInfo.pfnUserCallback = debugCallback;
}

std::vector<const char*> getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	for (const auto& extension : instanceExtensions) {
		extensions.push_back(extension);
	}

	return extensions;
}

bool checkValidationLayerSupport() {
	uint32_t layerCount;

	vk::enumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<vk::LayerProperties> availableLayers(layerCount);
	vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());


	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void VKEngine::createSwapchainFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vk::ImageView attachments[] = {
			swapChainImageViews[i]
		};

		vk::FramebufferCreateInfo framebufferInfo({});
		framebufferInfo.renderPass = swapchainRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		swapChainFramebuffers[i] = devContext.device.createFramebuffer(framebufferInfo);
	}
}

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
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;

		textureSampler_linear = devContext.device.createSampler(samplerInfo);
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

		textureSampler_nearest = devContext.device.createSampler(samplerInfo);
	}
}

void VKEngine::initWindow(const WindowSetting& settings, bool visible) {
	glfwInit();

	// even when starting up in fullscreen, these values should be set as the size to transition to when changing to windowed mode
	assert(settings.windowSizeX != 0 && settings.windowSizeY != 0);
	pre_fullscren_windowSizeX = settings.windowSizeX;
	pre_fullscren_windowSizeY = settings.windowSizeY;

	currentWindowMode = settings.windowMode;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	glfw_initial_primary_monitor_redBits = mode->redBits;
	glfw_initial_primary_monitor_greenBits = mode->greenBits;
	glfw_initial_primary_monitor_blueBits = mode->blueBits;
	glfw_initial_primary_monitor_refreshRate = mode->refreshRate;
	glfw_initial_primary_monitor_width = mode->width;
	glfw_initial_primary_monitor_height= mode->height;


	// The following logic assumes 3 possible window modes
	static_assert((int)WindowMode::WindowModeCount == 3);

	if (settings.windowMode == WindowMode::Fullscreen) {
		window = glfwCreateWindow(mode->width, mode->height, settings.name.c_str(), monitor, nullptr);
	}
	else if (settings.windowMode == WindowMode::Borderless) {
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		window = glfwCreateWindow(mode->width, mode->height, settings.name.c_str(), monitor, nullptr);
	}
	else { // windowed
		window = glfwCreateWindow(settings.windowSizeX, settings.windowSizeY, settings.name.c_str(), nullptr, nullptr);
	}
}

void VKEngine::updateWindow(WindowSetting& settings) {

	if (settings.windowMode == currentWindowMode)
		return;

	// transitioning away from windowed mode, so save orignal position and size
	if (currentWindowMode == WindowMode::Windowed) {
		glfwGetWindowPos(window, &pre_fullscren_windowPosX, &pre_fullscren_windowPosY);
		glfwGetWindowSize(window, &pre_fullscren_windowSizeX, &pre_fullscren_windowSizeY);
	}

	currentWindowMode = settings.windowMode;

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (settings.windowMode == WindowMode::Fullscreen) {
		// I'm not sure if this is required, but I don't know how to invalidate the last call to glfwWindowHint when setting borderless mode.
		// If it is reset between glfwSetWindow and glfwCreate window calls, then this is not neccassary. Need to try and get windows to use exclusive fullscreen.
		glfwDefaultWindowHints();
		glfwSetWindowMonitor(window, monitor, 0, 0, glfw_initial_primary_monitor_width, glfw_initial_primary_monitor_height, GLFW_DONT_CARE);
	}
	else if (settings.windowMode == WindowMode::Borderless) {
		glfwWindowHint(GLFW_RED_BITS, glfw_initial_primary_monitor_redBits);
		glfwWindowHint(GLFW_GREEN_BITS, glfw_initial_primary_monitor_greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, glfw_initial_primary_monitor_blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, glfw_initial_primary_monitor_refreshRate);
		glfwSetWindowMonitor(window, monitor, 0, 0, glfw_initial_primary_monitor_width, glfw_initial_primary_monitor_height, GLFW_DONT_CARE);
	}
	else { // windowed
		glfwSetWindowMonitor(window, nullptr, pre_fullscren_windowPosX, pre_fullscren_windowPosY, pre_fullscren_windowSizeX, pre_fullscren_windowSizeY, GLFW_DONT_CARE);
	}	
}

bool VKEngine::shouldClose() {
	return glfwWindowShouldClose(window);
}

void VKEngine::initVulkan(const SwapChainSetting& setting, int subPassCount) {
	createInstance();
	dynamicDispatcher = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);

	// setup debug messager
	if (enableValidationLayers) {

		vk::DebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &debugMessenger, dynamicDispatcher);

	}

	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();


	{
		VmaVulkanFunctions func = {};
		func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		func.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorInfo = {};
		//allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = devContext.device;
		allocatorInfo.instance = instance;
		allocatorInfo.pVulkanFunctions = &func;
		vmaCreateAllocator(&allocatorInfo, &allocator);
	}

	createSyncObjects();
	createSwapChain(setting);
	createRenderPass(swapchainRenderPass, swapChainImageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR, subPassCount); // on swap chain image format
	createSwapchainFramebuffers(); // depends on render pass

	// first frame has no previous submission to wait for
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		computeSubmittedFlags[i].store(true);
		graphicsSubmittedFlags[i].store(true);
	}

	queueSubmitThreadCancel.store(false);
	queueSubmitThread = std::thread(&VKEngine::queueSubmitThreadWorkFunc, this);
}

void VKEngine::GetWindowEvents() {
	glfwPollEvents();
}

void VKEngine::cleanup() {

	cancelSubmitThread();

	//cleanupSwapChain();

	//vkDestroyPipeline(device, graphicsPipeline, nullptr);
	//vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	//vkDestroyRenderPass(device, renderPass, nullptr);

	//for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
	//	vkDestroyBuffer(device, uniformBuffers[i], nullptr);
	//	vmaUnmapMemory(allocator, uniformBuffersAllocation[i]);
	//	vmaFreeMemory(allocator, uniformBuffersAllocation[i]);
	//}

	//vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	//vkDestroySampler(device, textureSampler, nullptr);
	//vkDestroyImageView(device, textureImageView, nullptr);

	//vkDestroyImage(device, textureImage, nullptr);
	//vmaFreeMemory(allocator, textureImageAllocation);

	//vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	//vkDestroyBuffer(device, indexBuffer, nullptr);
	//vmaFreeMemory(allocator, indexBufferAllocation);

	//vkDestroyBuffer(device, vertexBuffer, nullptr);
	//vmaFreeMemory(allocator, vertexBufferAllocation);

	//for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
	//	vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
	//	vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
	//	vkDestroyFence(device, inFlightFences[i], nullptr);
	//}

	//vkDestroyCommandPool(device, commandPool, nullptr);

	//vkDestroyDevice(device, nullptr);

	//if (enableValidationLayers) {
	////	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	// 
	// 	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	//if (func != nullptr) {
	//	func(instance, debugMessenger, pAllocator);
	//}
	// 
	// 
	//}

	//vkDestroySurfaceKHR(instance, surface, nullptr);
	//vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}

bool checkInstanceExtensionSupport();

void VKEngine::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = APPLICATION_NAME;
	appInfo.applicationVersion = ENGINE_VERSION;
	appInfo.pEngineName = ENGINE_NAME;
	appInfo.engineVersion = ENGINE_VERSION;
	appInfo.apiVersion = ENGINE_API_VERSION;

	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;

	if (checkInstanceExtensionSupport() == false) {
		throw std::runtime_error("Requried instance extensions not supported!");
	}

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	instance = vk::createInstance(createInfo);
}


QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;	

	auto queueFamilies = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {

		if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) && (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
			indices.graphicsAndComputeFamily = i;
		}

		vk::Bool32 presentSupport = false;
		device.getSurfaceSupportKHR(i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		//if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
		//	indices.graphicsFamily = i;
		//}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

bool checkInstanceExtensionSupport() {

	std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();

	std::set<std::string> requiredExtensions(instanceExtensions.begin(), instanceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

struct extensionSupportStatus {
	bool requiredSupport;
};

extensionSupportStatus checkDeviceExtensionSupport(vk::PhysicalDevice device) {

	std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

	std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	extensionSupportStatus support;
	support.requiredSupport = requiredExtensions.empty();
	return support;
}

struct deviceSuitabilityStatus {
	bool baseSuitability = false;
	bool discrete = false;
	int computeCapability = 0;
};
deviceSuitabilityStatus getDeviceSuitability(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	extensionSupportStatus extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported.requiredSupport) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	deviceSuitabilityStatus status;

	status.baseSuitability = true;
	// evaluate other optional feature here

	auto properties = device.getProperties();
	status.discrete = properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;

	// for now concider "compute capability" to be infered from the max texture size supported for a basic
	// idea of the device performance
	status.computeCapability = properties.limits.maxImageDimension2D;

	status.baseSuitability &= indices.isComplete();
	status.baseSuitability &= extensionsSupported.requiredSupport;
	status.baseSuitability &= swapChainAdequate;

	// 1.1 physical devices features
	{
		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceFeatures(device, &device_features);

		status.baseSuitability &= device_features.samplerAnisotropy;
	}

	// 1.2 physical device features
	{
		VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr };
		VkPhysicalDeviceFeatures2 device_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &indexing_features };
		vkGetPhysicalDeviceFeatures2(device, &device_features);

		status.baseSuitability &= indexing_features.descriptorBindingPartiallyBound;
		status.baseSuitability &= indexing_features.runtimeDescriptorArray;
	}

	return status;
}

void VKEngine::pickPhysicalDevice() {

	std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

	deviceSuitabilityStatus mostSuitableStatus;
	for (size_t i = 0; i < devices.size(); i++)
	{
		auto status = getDeviceSuitability(devices[i], surface);
		if (status.baseSuitability == false)
			continue;

		// prioritise minimum requirements, discrete devices, then compute capability
		if (!mostSuitableStatus.baseSuitability) {
			mostSuitableStatus = status;
			physicalDevice = devices[i];
			continue;
		}
		if (!mostSuitableStatus.discrete && status.discrete) {
			mostSuitableStatus = status;
			physicalDevice = devices[i];
			continue;
		}
		if (mostSuitableStatus.computeCapability < status.computeCapability) {
			mostSuitableStatus = status;
			physicalDevice = devices[i];
			continue;
		}
	}

	if (!physicalDevice) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

#if 0
bool CheckDeviceFeaturesSupported_descriptorBindingSampledImageUpdateAfterBind(vk::PhysicalDevice physicalDevice) {

	bool found = false;
	for (const auto& extension : instanceExtensions) {
		if (strcmp(extension, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0) {
			found = true;
		}
	}

	if (found == false) {
		throw std::runtime_error("cannot query devContext.device feature if VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2 not enabled!");
	}

	vk::PhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures;

	vk::PhysicalDeviceFeatures2 deviceFeatures2;
	deviceFeatures2.pNext = &indexingFeatures;

	// Get the features of the physical devContext.device
	physicalDevice.getFeatures2(&deviceFeatures2);

	return indexingFeatures.descriptorBindingSampledImageUpdateAfterBind;
}
#endif

void VKEngine::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	devContext.queueFamilyIndices = indices;

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsAndComputeFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	{
		VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr };
		VkPhysicalDeviceFeatures2 physical_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &indexing_features };
		vkGetPhysicalDeviceFeatures2(physicalDevice, &physical_features2);

		vk::DeviceCreateInfo createInfo({});

		std::vector<const char*> enabledExtensions;
		for (auto& e : requiredDeviceExtensions)
			enabledExtensions.push_back(e);

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
		createInfo.pNext = &physical_features2;

		// This should be already set to VK_TRUE, as we queried before.
		indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
		indexing_features.runtimeDescriptorArray = VK_TRUE;
		physical_features2.pNext = &indexing_features;


		vk::PhysicalDeviceSynchronization2FeaturesKHR synchronizationFeatures;
		synchronizationFeatures.synchronization2 = true;
		indexing_features.pNext = &synchronizationFeatures;

		vk::PhysicalDeviceSamplerYcbcrConversionFeatures ycbcrConversionFeatures;
		ycbcrConversionFeatures.samplerYcbcrConversion = true;
		synchronizationFeatures.pNext = &ycbcrConversionFeatures;

		devContext.device = physicalDevice.createDevice(createInfo);
	}


	devContext.device.getQueue(indices.graphicsAndComputeFamily.value(), 0, &devContext.graphicsQueue);
	devContext.device.getQueue(indices.graphicsAndComputeFamily.value(), 0, &devContext.computeQueue);
	devContext.device.getQueue(indices.presentFamily.value(), 0, &devContext.presentQueue);
}


void VKEngine::createSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}


vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes, SwapChainSetting setting) {

	// this logic translates setting to present mode with the assumption of there only being 2 frames in flight
	static_assert(FRAMES_IN_FLIGHT == 2, "FRAMES_IN_FLIGHT must be 2 for correct swap chain setting logic");

	vk::PresentModeKHR requestedMode;
	if (setting.capFramerate == false) {
		if (setting.vsync == true)
			requestedMode = vk::PresentModeKHR::eMailbox;
		else
			requestedMode = vk::PresentModeKHR::eImmediate;
	}
	else {
		if (setting.vsync == true)
			requestedMode = vk::PresentModeKHR::eFifo;
		else
			requestedMode = vk::PresentModeKHR::eFifoRelaxed;
	}

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == requestedMode) {
			return availablePresentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		vk::Extent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	SwapChainSupportDetails details;

	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);

	details.formats = device.getSurfaceFormatsKHR(surface);

	details.presentModes = device.getSurfacePresentModesKHR(surface);

	return details;
}

void VKEngine::createSwapChain(SwapChainSetting setting) {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, setting);
	vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount;

	// mailbox should have at least 1 extra image than the minimum to allow uncapped framerates
	if (presentMode == vk::PresentModeKHR::eMailbox)
		imageCount = std::max(imageCount, (uint32_t)(FRAMES_IN_FLIGHT + 1));
	else
		imageCount = std::max(imageCount, (uint32_t)(FRAMES_IN_FLIGHT));

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsAndComputeFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsAndComputeFamily != indices.presentFamily) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	swapChain = devContext.device.createSwapchainKHR(createInfo);

	swapChainImages = devContext.device.getSwapchainImagesKHR(swapChain);

	assert(imageCount <= MAX_SWAPCHAIN_IMAGES);

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	lastUsedSwapChainSetting = setting;

	// create image views
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (uint32_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
		}
	}
}

void VKEngine::recreateSwapChain(const SwapChainSetting& setting) {
	std::lock_guard<std::mutex> lock(queueSubmitMutex);

	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);

	// minimized. Wait for forground
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(devContext.device);

	cleanupSwapChain();
	createSwapChain(setting);

	{
		swapChainImageViews.resize(swapChainImages.size());

		for (uint32_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
		}
	}

	createSwapchainFramebuffers();
	swapchainOutOfDate.store(false);
}

void VKEngine::cleanupSwapChain() {
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(devContext.device, framebuffer, nullptr);
	}

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(devContext.device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(devContext.device, swapChain, nullptr);
}

void VKEngine::createCommandBuffers() {
	// graphics command buffers
	{
		vk::CommandBufferAllocateInfo allocInfo;
		allocInfo.commandPool = defaultThreadContext.commandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = (uint32_t)graphicsCommandBuffers.size();


		//auto res = devContext.device.allocateCommandBuffers(&allocInfo, graphicsCommandBuffers.data());
		//assert(res == vk::Result::eSuccess);
		// standard c++ header overload didn't work
		auto cmdBV = devContext.device.allocateCommandBuffers(allocInfo);
		for (size_t i = 0; i < graphicsCommandBuffers.size(); i++) {
			graphicsCommandBuffers[i] = cmdBV[i];
		}
	}
	// compute command buffers
	{
		vk::CommandBufferAllocateInfo allocInfo;
		allocInfo.commandPool = defaultThreadContext.commandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();


		/*auto res = devContext.device.allocateCommandBuffers(&allocInfo, computeCommandBuffers.data());
		assert(res == vk::Result::eSuccess);*/
		// standard c++ header overload didn't work
		auto cmdBV = devContext.device.allocateCommandBuffers(allocInfo);
		for (size_t i = 0; i < computeCommandBuffers.size(); i++) {
			computeCommandBuffers[i] = cmdBV[i];
		}
	}
}

#ifdef TRACY_ENABLE
void VKEngine::initTracyContext() {
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		tracyComputeContexts[i] = TracyVkContext(physicalDevice, devContext.device, devContext.computeQueue, computeCommandBuffers[i]);
		tracyGraphicsContexts[i] = TracyVkContext(physicalDevice, devContext.device, devContext.graphicsQueue, graphicsCommandBuffers[i]);
	}
}
#endif