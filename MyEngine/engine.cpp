#include <vector>
#include <stdint.h>
#include <memory>
#include <cassert>
#include <unordered_map>
#include <random>

#include <vulkan/vulkan.hpp>
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
#include "TextPL.h"

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

void Engine::Start(const VideoSettings& initialSettings) {


	PROFILE_START(Engine_Startup);

	rengine->initWindow(initialSettings.windowSetting, false);
	glfwSetWindowUserPointer(rengine->window, this);
	glfwSetFramebufferSizeCallback(rengine->window, framebufferResizeCallback);
	glfwSetKeyCallback(rengine->window, KeyCallback);
	glfwSetMouseButtonCallback(rengine->window, mouseButtonCallback);
	glfwSetScrollCallback(rengine->window, scroll_callback);

	input = make_shared<Input>(rengine->window);
	Entity::input = input;

	DebugLog("Initialized Window");

	rengine->initVulkan(initialSettings.swapChainSetting, 1);
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

	this->winW = initialSettings.windowSetting.windowSizeX;
	this->winH = initialSettings.windowSetting.windowSizeY;
	lastwinW = winW;
	lastwinH = winH;

	vector<uint8_t> checkerData;
	genCheckerboard(400, vec4(1, 0, 0, 1), vec4(0, 0, 1, 1), 6, checkerData);
	texNotFound = rengine->genTexture(400, 400, checkerData, FilterMode::Nearest);

	// create some buffers
	{
		cameraUploader.CreateBuffers(rengine);
		screenSpaceTransformUploader.CreateBuffers(rengine);
		AllocateQuad(rengine, quadMeshBuffer);
		worldMap = make_shared<TileWorld>(rengine);
		worldMap->AllocateVulkanResources();
	}

	// contruct pipelines
	{
		texturePipeline = make_unique<TexturedQuadPL>(rengine, texNotFound);
		colorPipeline = make_unique<ColoredQuadPL>(rengine);
		screenSpaceColorPipeline = make_unique<ColoredQuadPL>(rengine);
		tilemapPipeline = make_unique<TilemapPL>(rengine, worldMap);
		lightingPipeline = make_unique<LightingComputePL>(rengine, worldMap);
		textPipeline = make_unique<TextPL>(rengine, texNotFound);
		screenSpaceTexturePipeline = make_unique<TexturedQuadPL>(rengine, texNotFound);
		screenSpaceTextPipeline = make_unique<TextPL>(rengine, texNotFound);
	}

	// section can be done in parrallel

	{

		colorPipeline->CreateInstancingBuffer();
		texturePipeline->createSSBOBuffer();
		textPipeline->createSSBOBuffer();
		lightingPipeline->createStagingBuffers();
		screenSpaceColorPipeline->CreateInstancingBuffer();
		screenSpaceTexturePipeline->createSSBOBuffer();
		screenSpaceTextPipeline->createSSBOBuffer();

		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("color_vert.spv", vert);
			assetManager->LoadShaderFile("color_frag.spv", frag);
			colorPipeline->CreateGraphicsPipeline(vert, frag, cameraUploader.transferBuffers);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("texture_vert.spv", vert);
			assetManager->LoadShaderFile("texture_frag.spv", frag);
			texturePipeline->CreateGraphicsPipeline(vert, frag, cameraUploader.transferBuffers);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("tilemap_vert.spv", vert);
			assetManager->LoadShaderFile("tilemap_frag.spv", frag);
			tilemapPipeline->CreateGraphicsPipeline(vert, frag, cameraUploader.transferBuffers);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("text_vert.spv", vert);
			assetManager->LoadShaderFile("text_frag.spv", frag);
			textPipeline->CreateGraphicsPipeline(vert, frag, cameraUploader.transferBuffers);
		}
		{
			vector<uint8_t> comp, blur;
			assetManager->LoadShaderFile("lighting_comp.spv", comp);
			assetManager->LoadShaderFile("lightingBlur_comp.spv", blur);
			lightingPipeline->CreateComputePipeline(comp, blur);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("screenSpaceTexture_vert.spv", vert);
			assetManager->LoadShaderFile("screenSpaceTexture_frag.spv", frag);
			screenSpaceTexturePipeline->CreateGraphicsPipeline(vert, frag, screenSpaceTransformUploader.transferBuffers, true);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("screenSpaceShape_vert.spv", vert);
			assetManager->LoadShaderFile("screenSpaceShape_frag.spv", frag);
			screenSpaceColorPipeline->CreateGraphicsPipeline(vert, frag, screenSpaceTransformUploader.transferBuffers, true);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("screenSpaceText_vert.spv", vert);
			assetManager->LoadShaderFile("screenSpaceText_frag.spv", frag);
			screenSpaceTextPipeline->CreateGraphicsPipeline(vert, frag, screenSpaceTransformUploader.transferBuffers, true);
		}
	}

	rengine->createSyncObjects();


	InitImgui(); // imgui needs to be initialized to register a sprite with it

	genCheckerboard(400, vec4(1, 1, 0, 1), vec4(0, 0, 1, 1), 6, checkerData);
	assetManager->CreateDefaultSprite(400, 400, checkerData);

	DebugLog("Initialized pipelines");

	// no point in showing window before vulkan initilized
	glfwShowWindow(rengine->window);

	_onWindowResize();

	PROFILE_END(Engine_Startup);
}


//mat3 translate(vec2 v) {
//	return mat3(
//		1.0, 0.0, 0.0,
//		0.0, 1.0, 0.0,
//		v.x, v.y, 1.0
//	);
//}
//mat3 scale(vec2 v) {
//	return mat3(
//		v.x, 0.0, 0.0,
//		0.0, v.y, 0.0,
//		0.0, 0.0, 1.0
//	);
//}
//mat3 rotate(float angle) {
//	float c = cos(angle);
//	float s = sin(angle);
//	return mat3(
//		c, s, 0.0,
//		-s, c, 0.0,
//		0.0, 0.0, 1.0
//	);
//}


void Engine::ApplyNewVideoSettings(const VideoSettings settings) {
	newVideoSettingsRequested = true;
	requestedSettings = settings;
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

	runningStats.sprite_render_count = 0;

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

		for (auto& body : currentScene->sceneData.rigidbodies)
		{
			currentScene->sceneData.entities[body.first]->transform.position = body.second._getPosition();
			currentScene->sceneData.entities[body.first]->transform.rotation = body.second._getRotation();
		}
	}

	// update global buffers
	{
		ZoneScopedN("UBO updates");

		cameraUploader.Invalidate();
		cameraUBO_s camData;
		camData.aspectRatio = (float)winH / (float)winW;
		camData.position = camera.position;
		camData.zoom = camera.zoom;
		cameraUploader.SyncBufferData(camData, rengine->currentFrame);

		cameraUBO_s screenSpaceData;
		screenSpaceData.aspectRatio = (float)winH / (float)winW;
		//screenSpaceData.position = vec2((float)winW / 2.0f, -(float)winH / 2.0f);
		screenSpaceData.position = vec2((float)winW / 2.0f, -(float)winH / 2.0f);
		screenSpaceData.zoom = 2.0f / (float)winH;
		screenSpaceTransformUploader.SyncBufferData(screenSpaceData, rengine->currentFrame);
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




	uint32_t imageIndex = rengine->waitForSwapchain((newVideoSettingsRequested ? &requestedSettings.windowSetting : nullptr));
	newVideoSettingsRequested = false;
	if (imageIndex == -1) {
		// swapchain invalid - recreate
		frameCounter = 0;
		return false;
	}


	auto cmdBuffer = rengine->getNextCommandBuffer();

	rengine->beginRenderpass(imageIndex, cmdBuffer, vec4(0.0, 0.4, 0.6, 1.0));

	{
		vk::Viewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)rengine->swapChainExtent.width;
		viewport.height = (float)rengine->swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		cmdBuffer.setViewport(0, 1, &viewport);

		vk::Rect2D scissor;
		scissor.offset = vk::Offset2D { 0, 0 };
		scissor.extent = rengine->swapChainExtent;
		cmdBuffer.setScissor(0, 1, &scissor);

		vk::Buffer vertexBuffers[] = { quadMeshBuffer.vertexBuffer };
		vk::DeviceSize offsets[] = { 0 };
		cmdBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
		cmdBuffer.bindIndexBuffer(quadMeshBuffer.indexBuffer, 0, vk::IndexType::eUint16);
	}


	// colored quad
	{
		ZoneScopedN("Colored quad PL");

		vector<ColoredQuadPL::InstanceBufferData> drawlist;
		drawlist.reserve(currentScene->sceneData.colorRenderers.size());

		for (auto& renderer : currentScene->sceneData.colorRenderers)
		{
			const auto& entity = currentScene->sceneData.entities[renderer.first];

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

		if (assetChangeFlags->SpritesAdded()) {
			for (auto& [sprID, sprite] : assetManager->_getSpriteIterator())
				texturePipeline->addTextureBinding(sprite->textureID, resourceManager->GetTexture(sprite->textureID));
		}

		if (assetChangeFlags->TextureFiltersChanged())
			texturePipeline->invalidateTextureDescriptors();

		if (currentScene->sceneData.spriteRenderers.size() > 0) {
			vector<TexturedQuadPL::ssboObjectInstanceData> drawlist;
			drawlist.reserve(currentScene->sceneData.spriteRenderers.size());

			for (auto& [entID, renderer] : currentScene->sceneData.spriteRenderers)
			{
				const auto& entity = currentScene->sceneData.entities[entID];

				auto s = assetManager->GetSprite(renderer.sprite);

				TexturedQuadPL::ssboObjectInstanceData drawObject;
				drawObject.uvMin = vec2(0.0f);
				drawObject.uvMax = vec2(1.0f);
				drawObject.translation = entity->transform.position;
				drawObject.scale = entity->transform.scale;
				drawObject.rotation = entity->transform.rotation;
				drawObject.tex = s->textureID;

				if (s->atlas.size() > 0) {
					auto atEntry = s->atlas[renderer.atlasIndex];
					drawObject.uvMin = atEntry.uv_min;
					drawObject.uvMax = atEntry.uv_max;
				}

				drawlist.push_back(drawObject);
			}

			texturePipeline->updateDescriptorSets();
			texturePipeline->UploadInstanceData(drawlist);
			texturePipeline->recordCommandBuffer(cmdBuffer, drawlist.size());

			runningStats.sprite_render_count += drawlist.size();
		}
	}

	// text
	{
		ZoneScopedN("Text PL");

		if (assetChangeFlags->FontsAdded()) {
			for (auto& [fntID, font] : assetManager->_getFontIterator()) {
				auto sprite = assetManager->GetSprite(font->atlas);
				textPipeline->addFontBinding(font->ID, resourceManager->GetTexture(sprite->textureID));
			}
		}

		if (assetChangeFlags->TextureFiltersChanged())
			textPipeline->InvalidateTextureDescriptors();

		if (currentScene->sceneData.textRenderers.size() > 0) {
			textPipeline->ClearTextData(rengine->currentFrame);

			int i = 0;
			for (auto& [entID, r] : currentScene->sceneData.textRenderers) {

				const auto& entity = currentScene->sceneData.entities[entID];

				shared_ptr<Font> f = assetManager->GetFont(r.font);

				if (r.dirty) {
					r.quads.clear();
					r.quads.resize(r.text.length());
					CalculateQuads(f, r.text, r.quads.data());
					r.dirty = false;
				}

				TextPL::textHeader header;
				header.color = r.color;
				header.position = entity->transform.position;
				header.rotation = entity->transform.rotation;
				header.scale = entity->transform.scale;
				header.textLength = glm::min(TEXTPL_maxTextLength, (int)r.quads.size());

				//TODO: could potentially assume this data is already here if the renderer is not dirty ?
				TextPL::textObject textData;
				std::copy(r.quads.begin(), r.quads.end(), textData.quads);

				textPipeline->UploadTextData(rengine->currentFrame, i, header, r.font, textData);
				i++;
			}

			textPipeline->updateDescriptorSets();
			textPipeline->recordCommandBuffer(cmdBuffer);
		}
	}

	runningStats.entity_count = currentScene->sceneData.entities.size();

	// screenspace quad
	{
		screenSpaceColorPipeline->UploadInstanceData(screenSpaceColorDrawlist);
		screenSpaceColorPipeline->recordCommandBuffer(cmdBuffer, screenSpaceColorDrawlist.size());

	}
	// screenspace texture
	{
		if (assetChangeFlags->SpritesAdded()) {
			for (auto& [sprID, sprite] : assetManager->_getSpriteIterator())
				screenSpaceTexturePipeline->addTextureBinding(sprite->textureID, resourceManager->GetTexture(sprite->textureID));
		}

		if (assetChangeFlags->TextureFiltersChanged())
			screenSpaceTexturePipeline->invalidateTextureDescriptors();

		screenSpaceTexturePipeline->updateDescriptorSets();
		screenSpaceTexturePipeline->UploadInstanceData(screenSpaceTextureDrawlist);
		screenSpaceTexturePipeline->recordCommandBuffer(cmdBuffer, screenSpaceTextureDrawlist.size());

		runningStats.sprite_render_count += screenSpaceTextureDrawlist.size();
	}

	// screenspace text
	{
		if (assetChangeFlags->FontsAdded()) {
			for (auto& [fntID, font] : assetManager->_getFontIterator()) {
				auto sprite = assetManager->GetSprite(font->atlas);
				screenSpaceTextPipeline->addFontBinding(font->ID, resourceManager->GetTexture(sprite->textureID));
			}
		}

		if (assetChangeFlags->TextureFiltersChanged())
			screenSpaceTextPipeline->InvalidateTextureDescriptors();

		int memSlot = 0;
		for (auto& i : screenSpaceTextDrawlist)
		{
			shared_ptr<Font> f = assetManager->GetFont(i.font);
			TextPL::textObject textData;

			CalculateQuads(f, i.text, textData.quads);

			i.header.scale = vec2(f->fontHeight * 2) * i.scaleFactor;

			screenSpaceTextPipeline->UploadTextData(rengine->currentFrame, memSlot++, i.header, i.font, textData);
		}
		screenSpaceTextPipeline->updateDescriptorSets();
		screenSpaceTextPipeline->recordCommandBuffer(cmdBuffer);
	}



	// reset binding flags
	assetChangeFlags->ClearSpritesAdded();
	assetChangeFlags->ClearFontsAdded();
	assetChangeFlags->ClearTextureFilterschanged();

	if (drawImgui) {
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	}

	vkCmdEndRenderPass(cmdBuffer);
	rengine->endCommandBuffer(cmdBuffer);

	if (rengine->submitAndPresent(imageIndex)) {
		_onWindowResize();
	}

	rengine->Update();

	input->_newFrame();

	frameCounter++;

	firstFrame = false;

	if (lastwinH != winH || lastwinW != winW) {
		windowResizedLastFrame = true;
	}
	else {
		windowResizedLastFrame = false;
	}
	lastwinW = winW;
	lastwinH = winH;


	FrameMark;
	return true;
}

bool Engine::ShouldClose() {
	return rengine->shouldClose();
}

void Engine::Close() {
	rengine->cleanup();
}


void Engine::updatePhysics() {
	bworld->Step(timeStep, velocityIterations, positionIterations);
}




static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
//static void check_vk_result(vk::Result err)
//{
//	if (err == 0)
//		return;
//	fprintf(stderr, "[vulkan] Error: vk::Result = %d\n", err);
//	if (err < 0)
//		abort();
//}

#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_report(vk::DebugReportFlagsEXT flags, vk::DebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
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
	vk::DescriptorPoolSize pool_sizes[] = {
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 }
	};


	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	vk::DescriptorPool imguiPool;
	rengine->device.createDescriptorPool(&pool_info, nullptr, &imguiPool);


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
	init_info.MinImageCount = rengine->swapChainImages.size();
	init_info.ImageCount = rengine->swapChainImages.size();
	init_info.Subpass = 0;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, rengine->renderPass);


	//execute a gpu command to upload imgui font textures
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


