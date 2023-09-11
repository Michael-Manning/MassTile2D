#include <vector>
#include <stdint.h>
#include <memory>
#include <cassert>
#include <unordered_map>
#include <random>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <box2d/box2d.h>

#include <tracy/Tracy.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"

#include "coloredQuadPL.h"
#include "texturedQuadPL.h"
#include "tilemapPL.h"
#include "LightingComputePL.h"

#include "MyMath.h"
#include "engine.h"
#include "SpriteRenderer.h"	
#include "Physics.h"
#include "Entity.h"
#include "Vertex.h"
#include "profiling.h"
#include "Settings.h"

using namespace glm;
using namespace std;


namespace {

	void genCheckerboard(int size, glm::vec4 cola, glm::vec4 colb, int squares, std::vector<uint8_t>& data) {
		// Determine the size of each square
		int squareSize = size / squares;

		data.resize(size * size * 4);  // Assuming 4 bytes for RGBA

		for (size_t i = 0; i < size; i++)
		{
			for (size_t j = 0; j < size; j++)
			{
				// Determine which square the current pixel is in
				int squareX = i / squareSize;
				int squareY = j / squareSize;

				// Decide the color based on the parity of the square indices
				bool isEvenSquare = (squareX + squareY) % 2 == 0;

				glm::vec4 chosenColor = isEvenSquare ? cola : colb;

				// Convert the color to 8-bit and store in the data array
				size_t index = (i * size + j) * 4; // 4 bytes per pixel
				data[index] = static_cast<uint8_t>(chosenColor.r * 255);
				data[index + 1] = static_cast<uint8_t>(chosenColor.g * 255);
				data[index + 2] = static_cast<uint8_t>(chosenColor.b * 255);
				data[index + 3] = static_cast<uint8_t>(chosenColor.a * 255);
			}
		}
	}

	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator

	int ran(int min, int max) {
		std::uniform_int_distribution<> dis(min, max);
		return dis(gen);
	}
	double ran(double min, double max) {
		std::uniform_real_distribution<> dis(min, max);
		return dis(gen);
	}
}


// physics settings
b2Vec2 gravity(0.0f, -10.0f);
constexpr double timeStep = 1.0f / 60.0f;
int32 velocityIterations = 6;
int32 positionIterations = 2;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	app->hintWindowResize();
	app->winW = width;
	app->winH = height;
	app->_onWindowResize();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	app->GetInput()->_onKeyboard(key, scancode, action, mods);
}
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	app->GetInput()->_onMouseButton(button, action, mods);
}
void mouseMoveCallback(GLFWwindow* window, double x, double y) {
	auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
	app->GetInput()->_onScroll(xoffset, yoffset);
}

void Engine::Start(std::string windowName, int winW, int winH, std::string shaderDir, const SwapChainSetting swapchainSetting) {

	PROFILE_START(Engine_Startup);

	rengine->initWindow(winW, winH, windowName, false);
	glfwSetWindowUserPointer(rengine->window, this);
	glfwSetFramebufferSizeCallback(rengine->window, framebufferResizeCallback);
	glfwSetKeyCallback(rengine->window, KeyCallback);
	glfwSetMouseButtonCallback(rengine->window, mouseButtonCallback);

	glfwSetScrollCallback(rengine->window, scroll_callback);

	input = make_shared<Input>(rengine->window);
	Entity::input = input;

	DebugLog("Initialized Window");

	rengine->initVulkan(swapchainSetting, 1);


	rengine->createFramebuffers();
	rengine->createCommandPool();
	rengine->createDescriptorPool();
	rengine->createCommandBuffers();
	rengine->createTextureSamplers();

	DebugLog("Initialized Vulkan");

#ifdef TRACY_ENABLE
	rengine->initTracyContext();
	DebugLog("Initialized Tracy");
#endif

	this->winW = winW;
	this->winH = winH;

	vector<uint8_t> checkerData;
	genCheckerboard(400, vec4(1, 0, 0, 1), vec4(0, 0, 1, 1), 6, checkerData);
	texNotFound = rengine->genTexture(400, 400, checkerData, FilterMode::Nearest);

	// create some buffers
	{
		cameraUploader.CreateBuffers(rengine);
		AllocateQuad(rengine, quadMeshBuffer);
		worldMap = make_shared<TileWorld>(rengine);
		worldMap->AllocateVulkanResources();
	}

	// contruct pipelines
	{
		texturePipeline = make_unique<TexturedQuadPL>(rengine);
		colorPipeline = make_unique<ColoredQuadPL>(rengine);
		tilemapPipeline = make_unique<TilemapPL>(rengine, worldMap);
		lightingPipeline = make_unique<LightingComputePL>(rengine, worldMap);
	}

	// section can be done in parrallel



	// configure color pipeline
	{
		colorPipeline->CreateInstancingBuffer();
		colorPipeline->CreateGraphicsPipeline(shaderDir + "color_vert.spv", shaderDir + "color_frag.spv", cameraUploader.transferBuffers);
	}

	// configure instancing pipeline
	{
		texturePipeline->setDefaultTexture(texNotFound);
		texturePipeline->createDescriptorSetLayout();
		texturePipeline->CreateGraphicsPipeline(shaderDir + "texture_vert.spv", shaderDir + "texture_frag.spv");
		texturePipeline->createSSBOBuffer();
		texturePipeline->createDescriptorSets(cameraUploader.transferBuffers);
	}

	// tilemap pipeline
	{
		tilemapPipeline->CreateGraphicsPipeline(shaderDir + "tilemap_vert.spv", shaderDir + "tilemap_frag.spv", cameraUploader.transferBuffers);
	}

	// lighting compute pipeline
	{
		lightingPipeline->createStagingBuffers();
		lightingPipeline->CreateComputePipeline(shaderDir + "lighting_comp.spv", shaderDir + "lightingBlur_comp.spv");
	}

	rengine->createSyncObjects();
	initPhysics();


	InitImgui(); // imgui needs to be initialized to register a sprite with it

	genCheckerboard(400, vec4(1, 1, 0, 1), vec4(0, 0, 1, 1), 6, checkerData);
	assetManager->CreateDefaultSprite(400, 400, checkerData);

	DebugLog("Initialized pipelines");

	// no point in showing window before vulkan initilized
	glfwShowWindow(rengine->window);

	_onWindowResize();

	PROFILE_END(Engine_Startup);
}

void Engine::loadPrefabs() {
	assetManager->loadPrefabs(bworld);
	DebugLog("prefabs loaded");
}


mat3 translate(vec2 v) {
	return mat3(
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		v.x, v.y, 1.0
	);
}
mat3 scale(vec2 v) {
	return mat3(
		v.x, 0.0, 0.0,
		0.0, v.y, 0.0,
		0.0, 0.0, 1.0
	);
}
mat3 rotate(float angle) {
	float c = cos(angle);
	float s = sin(angle);
	return mat3(
		c, s, 0.0,
		-s, c, 0.0,
		0.0, 0.0, 1.0
	);
}

bool Engine::QueueNextFrame(bool drawImgui) {
	ZoneScopedN("queue next frame");

	if (firstFrame) {
		//glfwSetTime(0.0);
	}

	double time = glfwGetTime();
	deltaTime = time - lastTime;
	paused_deltaTime = deltaTime;
	framerate = 1.0 / deltaTime;
	frameTimes[frameTimeIndex++] = framerate;
	frameTimeIndex = frameTimeIndex % frameTimeBufferCount;

	lastTime = time;
	this->time = time;

	if (paused)
	{
		deltaTime = 0.0;
	}

	if (!paused) {
		physicsTimer += deltaTime;
	}

	{
		ZoneScopedN("Update physics");
		while (physicsTimer >= timeStep) {
			updatePhysics();
			physicsTimer -= timeStep;
		}
		for (auto& body : scene->sceneData.rigidbodies)
		{
			scene->sceneData.entities[body.first]->transform.position = body.second._getPosition();
			scene->sceneData.entities[body.first]->transform.rotation = body.second._getRotation();
		}
	}

	// update global buffers
	{
		ZoneScopedN("UBO update");

		cameraUploader.Invalidate();

		cameraUBO_s camData;
		camData.aspectRatio = (float)winH / (float)winW;
		camData.position = camera.position;
		camData.zoom = camera.zoom;

		cameraUploader.SyncBufferData(camData, rengine->currentFrame);

	}


	{
		ZoneScopedN("compute shaders");

		rengine->waitForCompute();
		auto computeCmdBuffer = rengine->getNextComputeCommandBuffer();

		// transfer tiles to read-only memory before starting renderpass
		{
			ZoneScopedN("chunk updates");
			worldMap->stageChunkUpdates(computeCmdBuffer);
		}

		auto lightingData = worldMap->getLightingUpdateData();
		lightingPipeline->stageLightingUpdate(lightingData);
		lightingPipeline->recordCommandBuffer(computeCmdBuffer, lightingData.size());
		worldMap->chunkLightingJobs.clear();
		rengine->submitCompute();
	}


	uint32_t imageIndex = rengine->waitForSwapchain();
	if (imageIndex == -1) {
		// swapchain invalid - recreate
		frameCounter = 0;
		return false;
	}


	auto cmdBuffer = rengine->getNextCommandBuffer();

	rengine->beginRenderpass(imageIndex, cmdBuffer, vec4(0.0, 0.4, 0.6, 1.0));

	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)rengine->swapChainExtent.width;
		viewport.height = (float)rengine->swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = rengine->swapChainExtent;
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { quadMeshBuffer.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, quadMeshBuffer.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	}



	// colored quad
	{
		ZoneScopedN("Colored quad PL");

		vector<ColoredQuadPL::InstanceBufferData> drawlist;
		drawlist.reserve(scene->sceneData.colorRenderers.size());

		for (auto& renderer : scene->sceneData.colorRenderers)
		{
			const auto& entity = scene->sceneData.entities[renderer.first];

			ColoredQuadPL::InstanceBufferData instanceData;

			instanceData.color = renderer.second.color;
			instanceData.position = entity->transform.position;
			instanceData.scale = entity->transform.scale;
			instanceData.circle = renderer.second.shape == ColorRenderer::Shape::Circle;
			instanceData.rotation = entity->transform.rotation;

			drawlist.push_back(instanceData);
		}

		colorPipeline->UploadInstanceData(drawlist);
		colorPipeline->recordCommandBuffer(cmdBuffer, drawlist.size());
	}

	// tilemap

	{
		//ZoneScopedN("tilemap PL");

		if (tilemapPipeline->textureAtlas.has_value()) {
			tilemapPipeline->recordCommandBuffer(cmdBuffer);
		}
	}

	// Textured quad
	{
		ZoneScopedN("Textured quad PL");

		if (scene->sceneData.spriteRenderers.size() > 0) {
			vector<TexturedQuadPL::ssboObjectInstanceData> drawlist;
			drawlist.reserve(scene->sceneData.spriteRenderers.size());

			for (auto& renderer : scene->sceneData.spriteRenderers)
			{
				const auto& entity = scene->sceneData.entities[renderer.first];

				auto s = assetManager->spriteAssets[renderer.second.sprite];
				if (assetManager->spritesAdded) {
					texturePipeline->addTextureBinding(s->texture, &assetManager->textureAssets[s->texture]);
				}

				TexturedQuadPL::ssboObjectInstanceData drawObject;
				drawObject.uvMin = vec2(0.0f);
				drawObject.uvMax = vec2(1.0f);
				drawObject.translation = entity->transform.position;
				drawObject.scale = entity->transform.scale;
				drawObject.rotation = entity->transform.rotation;
				drawObject.index = s->texture;

				if (s->Atlas.size() > 0) {
					auto atEntry = s->Atlas[renderer.second.atlasIndex];
					drawObject.uvMin = atEntry.uv_min;
					drawObject.uvMax = atEntry.uv_max;
				}

				drawlist.push_back(drawObject);
			}
			if (assetManager->filterModesChanged)
				texturePipeline->invalidateTextureDescriptors();

			texturePipeline->updateDescriptorSets();
			texturePipeline->UploadInstanceData(drawlist);
			texturePipeline->recordCommandBuffer(cmdBuffer, drawlist.size());

			runningStats.sprite_render_count = drawlist.size();
		}
	}

	runningStats.entity_count = scene->sceneData.entities.size();

	// reset binding flags
	assetManager->spritesAdded = false;
	assetManager->filterModesChanged = false;

	if (drawImgui) {
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	}

	vkCmdEndRenderPass(cmdBuffer);
	rengine->endCommandBuffer(cmdBuffer);



	rengine->submitAndPresent(imageIndex);
	

	rengine->Update();

	input->_newFrame();

	frameCounter++;

	firstFrame = false;


	FrameMark;
	return true;
}

bool Engine::ShouldClose() {
	return rengine->shouldClose();
}

void Engine::Close() {
	rengine->cleanup();
}



void Engine::initPhysics() {

	bworld = make_shared<b2World>(gravity);
	scene->setB2World(bworld);
}

void Engine::updatePhysics() {
	bworld->Step(timeStep, velocityIterations, positionIterations);
}




static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
	fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
	return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT


void Engine::InitImgui() {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls


	// Setup Dear ImGui style
	ImGui::StyleColorsDark();


	//1: create descriptor pool for IMGUI
	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	vkCreateDescriptorPool(rengine->device, &pool_info, nullptr, &imguiPool);


	// 2: initialize imgui library

	//this initializes the core structures of imgui
	ImGui::CreateContext();

	//this initializes imgui for SDL
	//ImGui_ImplSDL2_InitForVulkan(_window);
	ImGui_ImplGlfw_InitForVulkan(rengine->window, true);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = rengine->instance;
	init_info.PhysicalDevice = rengine->physicalDevice;
	init_info.Device = rengine->device;
	init_info.Queue = rengine->graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.Subpass = 0;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, rengine->renderPass);



	//execute a gpu command to upload imgui font textures
//	immediate_submit([&](VkCommandBuffer cmd) {
	//	ImGui_ImplVulkan_CreateFontsTexture(cmd);
		//});

	auto cmdBuffer = rengine->beginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
	rengine->endSingleTimeCommands(cmdBuffer);

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	//add the destroy the imgui created structures
	//_mainDeletionQueue.push_function([=]() {

	//	vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	//	ImGui_ImplVulkan_Shutdown();
	//	});


}

void stageChunkUpdates() {

}


