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
#include "GlobalImageDescriptor.h"

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

void Engine::Start(const VideoSettings& initialSettings, AssetManager::AssetPaths assetPaths) {


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
	rengine->createCommandPools();
	rengine->createDescriptorPool();
	rengine->createCommandBuffers();
	rengine->createTextureSamplers();

	resourceManager = std::make_shared<ResourceManager>(rengine, resourceChangeFlags);
	assetManager = std::make_shared<AssetManager>(rengine, assetPaths, resourceManager);

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
	texNotFoundID = resourceManager->GenerateTexture(400, 400, checkerData, FilterMode::Nearest, false);
	auto texNotFound = resourceManager->GetTexture(texNotFoundID);
	globalTextureBindingManager.AddBinding(texNotFoundID, texNotFound);

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
		texturePipeline = make_unique<TexturedQuadPL>(rengine);
		colorPipeline = make_unique<ColoredQuadPL>(rengine);
		screenSpaceColorPipeline = make_unique<ColoredQuadPL>(rengine);
		tilemapPipeline = make_unique<TilemapPL>(rengine, worldMap);
		lightingPipeline = make_unique<LightingComputePL>(rengine, worldMap);
		textPipeline = make_unique<TextPL>(rengine);
		screenSpaceTexturePipeline = make_unique<TexturedQuadPL>(rengine);
		screenSpaceTextPipeline = make_unique<TextPL>(rengine);
	}

	InitImgui(); // imgui needs to be initialized to register textures with it

	{
		GlobalTextureDesc.CreateLayout(0, max_bindless_resources);
		GlobalTextureDesc.CreateDescriptors();
	}

	// temp create main camera frambuffer
	{
		tmp_cameraFB = resourceManager->CreateFramebuffer({ winW , winH }, vec4(0.0, 0.4, 0.6, 1.0));
	}

	// section can be done in parrallel
	{
		auto tmpRP = resourceManager->GetFramebuffer(tmp_cameraFB)->renderpass;

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
			colorPipeline->CreateGraphicsPipeline(vert, frag, tmpRP, cameraUploader.transferBuffers);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("texture_vert.spv", vert);
			assetManager->LoadShaderFile("texture_frag.spv", frag);
			texturePipeline->CreateGraphicsPipeline(vert, frag, tmpRP, &GlobalTextureDesc, cameraUploader.transferBuffers);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("tilemap_vert.spv", vert);
			assetManager->LoadShaderFile("tilemap_frag.spv", frag);
			tilemapPipeline->CreateGraphicsPipeline(vert, frag, tmpRP, cameraUploader.transferBuffers);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("text_vert.spv", vert);
			assetManager->LoadShaderFile("text_frag.spv", frag);
			textPipeline->CreateGraphicsPipeline(vert, frag, tmpRP, &GlobalTextureDesc, cameraUploader.transferBuffers);
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
			screenSpaceTexturePipeline->CreateGraphicsPipeline(vert, frag, rengine->swapchainRenderPass, &GlobalTextureDesc, screenSpaceTransformUploader.transferBuffers, true);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("screenSpaceShape_vert.spv", vert);
			assetManager->LoadShaderFile("screenSpaceShape_frag.spv", frag);
			screenSpaceColorPipeline->CreateGraphicsPipeline(vert, frag, rengine->swapchainRenderPass, screenSpaceTransformUploader.transferBuffers, true);
		}
		{
			vector<uint8_t> vert, frag;
			assetManager->LoadShaderFile("screenSpaceText_vert.spv", vert);
			assetManager->LoadShaderFile("screenSpaceText_frag.spv", frag);
			screenSpaceTextPipeline->CreateGraphicsPipeline(vert, frag, rengine->swapchainRenderPass, &GlobalTextureDesc, screenSpaceTransformUploader.transferBuffers, true);
		}
	}

	genCheckerboard(400, vec4(1, 1, 0, 1), vec4(0, 0, 1, 1), 6, checkerData);
	assetManager->CreateDefaultSprite(400, 400, checkerData);

	DebugLog("Initialized pipelines");

	// no point in showing window before vulkan initilized
	glfwShowWindow(rengine->window);

	_onWindowResize();

	PROFILE_END(Engine_Startup);
}

void Engine::ApplyNewVideoSettings(const VideoSettings settings) {
	newVideoSettingsRequested = true;
	requestedSettings = settings;
}

bool Engine::QueueNextFrame(bool drawImgui) {
	ZoneScopedN("queue next frame");

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

	// wait for the this frame's compute command buffer to become available
	rengine->WaitForComputeSubmission();
	rengine->WaitForComputeCompletion();


	// record compute command buffer without submmiting it
	{
		ZoneScopedN("compute shaders");

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

		TracyVkCollect(rengine->tracyComputeContexts[rengine->currentFrame], rengine->computeCommandBuffers[rengine->currentFrame]);

		computeCmdBuffer.end();
	}


	// NOTE!!!!
	/*
	* We could submit the compute command buffer before waiting for the graphics buffer, 
	* but since the camera UBO is shared between the compute and graphics command buffers
	* we must wait for both to complete in the previous frame before updating them
	*/
	
	rengine->WaitForGraphicsSubmission();
	rengine->WaitForGraphicsCompletion();


	// update global buffers now that they are not in use by either queue, they can be updated
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
		screenSpaceData.position = vec2((float)winW / 2.0f, -(float)winH / 2.0f);
		screenSpaceData.zoom = 2.0f / (float)winH;
		screenSpaceTransformUploader.SyncBufferData(screenSpaceData, rengine->currentFrame);
	}

	// now free to execute compute buffer
	rengine->QueueComputeSubmission();

	
	resourceManager->ReleaseFreeableTextures();

	resourceManager->HandleFramebufferRecreation();


	{
		// free up indexes in bindless descriptor set
		while (textureBindingDeletionQueue.size() > 0) {
			auto& id = textureBindingDeletionQueue.front();
			textureBindingDeletionQueue.pop();
			if (globalTextureBindingManager.HasBinding(id)) // may have been deleted before it was finished uploading
				globalTextureBindingManager.RemoveBinding(id);
		}

		bool texturesAdded = resourceChangeFlags->TexturesAdded();
		if (texturesAdded) {
			globalTextureBindingManager.InvalidateDescriptors();
		}

		// mainly needed for asynchronous textures becoming available
		if (texturesAdded) {
			for (auto& [ID, tex] : *resourceManager->GetInternalTextureResources()) {

				// REMOVE ONCE TESTED
				assert(&tex == resourceManager->GetTexture(ID));

				globalTextureBindingManager.AddBinding(ID, (Texture*)&tex);
			}
		}

		if (resourceChangeFlags->TextureFiltersChanged())
			globalTextureBindingManager.InvalidateDescriptors();

		if (globalTextureBindingManager.IsDescriptorDirty(rengine->currentFrame) == true) {

			std::vector<int> indexes;
			std::vector <Texture*> textures;

			// TODO: Optimize use of bindless descriptors by only adding new textures to descriptor instead of all loaded textures.
			// Will need additional queues for each frame in flight to keep track of what textures have been fully uploaded and need to be bound.
			{
				std::unique_lock<std::mutex> lock(resourceManager->texMapMtx);
				for (auto& [ID, tex] : *resourceManager->GetInternalTextureResources()) {

					// REMOVE ONCE TESTED
					//assert(&tex == resourceManager->GetTexture(ID));

					if (globalTextureBindingManager.HasBinding(ID)) {

						indexes.push_back(globalTextureBindingManager.getIndexFromBinding(ID));
						auto tt = globalTextureBindingManager.getValueFromBinding(ID);
						textures.push_back(tt);
					}
				}
			}

			GlobalTextureDesc.AddDescriptors(indexes, textures, rengine->currentFrame);
			globalTextureBindingManager.ClearDescriptorDirty(rengine->currentFrame);
		}

		// use flag for thread safety
		if (texturesAdded)
			resourceChangeFlags->ClearTexturesAdded();

		resourceChangeFlags->ClearTextureFilterschanged();
	}

	auto cmdBuffer = rengine->getNextGfxCommandBuffer();

	rengine->beginRenderpass(resourceManager->GetFramebuffer(tmp_cameraFB), cmdBuffer);

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
		scissor.offset = vk::Offset2D{ 0, 0 };
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
		ZoneScopedN("tilemap PL");

		if (tilemapPipeline->textureAtlas.has_value()) {
			tilemapPipeline->recordCommandBuffer(cmdBuffer);
		}
	}

	// Textured quad
	{
		ZoneScopedN("Textured quad PL");

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
				drawObject.tex = globalTextureBindingManager.getIndexFromBinding(s->textureID);

				if (s->atlas.size() > 0) {
					auto atEntry = s->atlas[renderer.atlasIndex];
					drawObject.uvMin = atEntry.uv_min;
					drawObject.uvMax = atEntry.uv_max;
				}

				drawlist.push_back(drawObject);
			}

			texturePipeline->UploadInstanceData(drawlist);
			texturePipeline->recordCommandBuffer(cmdBuffer, drawlist.size());

			runningStats.sprite_render_count += drawlist.size();
		}
	}

	// text
	{
		ZoneScopedN("Text PL");

		if (currentScene->sceneData.textRenderers.size() > 0) {
			textPipeline->ClearTextData(rengine->currentFrame);

			int i = 0;
			for (auto& [entID, r] : currentScene->sceneData.textRenderers) {

				const auto& entity = currentScene->sceneData.entities[entID];

				shared_ptr<Font> f = assetManager->GetFont(r.font);
				auto sprite = assetManager->GetSprite(f->atlas);

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
				header._textureIndex = globalTextureBindingManager.getIndexFromBinding(sprite->textureID);

				//TODO: could potentially assume this data is already here if the renderer is not dirty ?
				TextPL::textObject textData;
				std::copy(r.quads.begin(), r.quads.end(), textData.quads);

				textPipeline->UploadTextData(rengine->currentFrame, i, header, r.font, textData);
				i++;
			}
			textPipeline->recordCommandBuffer(cmdBuffer);
		}
	}

	cmdBuffer.endRenderPass();

	{
		auto fb = resourceManager->GetFramebuffer(tmp_cameraFB);
		rengine->insertFramebufferTransitionBarrier(cmdBuffer, fb);

		addScreenCenteredSpaceFramebufferTexture(tmp_cameraFB, vec2(winW, winH) / 2.0f, winH, 0);
	}

	// we have to wait for the previous frame to finish presenting so we can determine what image the
	// next swapchain renderpass will be using
	uint32_t imageIndex;
	{
		//newVideoSettingsRequested = false;
		
		// wait for the presentation to be subbmited
		if (!firstFrame)
			rengine->WaitForFramePresentationSubmission();

		// this is the earliest point which we know for absolute certain that the window was resized as this flag is updated
		// when the present queue is submitted, although it could also be flagged before this point is reached
		if (rengine->ShouldRecreateSwapchain()) {
			// everything submitted for this frame is now in the wrong resolution and the swapchain must be resized 
			// before it can be presented again. We just resize it and display one stretched frame

			rengine->recreateSwapChain(rengine->lastUsedSwapChainSetting);

			// TODO verify that the glfw callback happens before this to ensure the new winW and winH values are updated
			resourceManager->ResizeFramebuffer(tmp_cameraFB, { winW, winH });

			_onWindowResize();
		}


		// then wait for the frame to be presented
		imageIndex = rengine->WaitForSwapChainImageAvailableAndHandleWindowChanges((newVideoSettingsRequested ? &requestedSettings.windowSetting : nullptr));
	}

	rengine->beginSwapchainRenderpass(imageIndex, cmdBuffer, vec4(0.0, 0.0, 0.0, 1.0));

	runningStats.entity_count = currentScene->sceneData.entities.size();

	// screenspace quad
	{
		screenSpaceColorPipeline->UploadInstanceData(screenSpaceColorDrawlist);
		screenSpaceColorPipeline->recordCommandBuffer(cmdBuffer, screenSpaceColorDrawlist.size());

	}

	// screenspace texture
	{
		screenSpaceTexturePipeline->UploadInstanceData(screenSpaceTextureDrawlist);
		screenSpaceTexturePipeline->recordCommandBuffer(cmdBuffer, screenSpaceTextureDrawlist.size());

		runningStats.sprite_render_count += screenSpaceTextureDrawlist.size();
	}

	// screenspace text
	{
		int memSlot = 0;
		for (auto& i : screenSpaceTextDrawlist)
		{
			shared_ptr<Font> f = assetManager->GetFont(i.font);
			auto sprite = assetManager->GetSprite(f->atlas);

			TextPL::textObject textData;

			CalculateQuads(f, i.text, textData.quads);

			i.header.scale = vec2(f->fontHeight * 2) * i.scaleFactor;
			i.header._textureIndex = globalTextureBindingManager.getIndexFromBinding(sprite->textureID);

			screenSpaceTextPipeline->UploadTextData(rengine->currentFrame, memSlot++, i.header, i.font, textData);
		}
		screenSpaceTextPipeline->recordCommandBuffer(cmdBuffer);
	}

	if (drawImgui) {
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
	}

	cmdBuffer.endRenderPass();
	TracyVkCollect(rengine->tracyGraphicsContexts[rengine->currentFrame], rengine->graphicsCommandBuffers[rengine->currentFrame]);
	rengine->endCommandBuffer(cmdBuffer);

	rengine->QueueGraphicsSubmission();

	rengine->QueuePresentSubmission(imageIndex);

	rengine->IncrementFrameInFlight();

	rengine->GetWindowEvents();

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


void Engine::updatePhysics() {
	bworld->Step(timeStep, velocityIterations, positionIterations);
}




static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

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
	rengine->devContext.device.createDescriptorPool(&pool_info, nullptr, &imguiPool);


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
	init_info.Device = rengine->devContext.device;
	init_info.Queue = rengine->devContext.graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = rengine->swapChainImages.size();
	init_info.ImageCount = rengine->swapChainImages.size();
	init_info.Subpass = 0;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, rengine->swapchainRenderPass);


	//execute a gpu command to upload imgui font textures
	auto cmdBuffer = rengine->beginSingleTimeCommands_gfx();
	ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
	rengine->endSingleTimeCommands_gfx(cmdBuffer);

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	//add the destroy the imgui created structures
	//_mainDeletionQueue.push_function([=]() {

	//	vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	//	ImGui_ImplVulkan_Shutdown();
	//	});


}

//void stageChunkUpdates() {
//
//}
//

