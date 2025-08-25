#include "stdafx.h"

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

#include "texture.h"
#include "VKEngine.h"

#include "pipelines.h"

#include "MyMath.h"
#include "engine.h"
#include "SpriteRenderer.h"	
#include "Physics.h"
#include "Entity.h"
#include "Vertex2D.h"
#include "profiling.h"
#include "Settings.h"
#include "GlobalImageDescriptor.h"

PROFILING_IMPL


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

void Engine::initializeSceneGraphicsContext(SceneGraphicsContext& ctx, glm::ivec2 framebufferSize) {

	PROFILE_SCOPE;

	ctx.drawFramebuffer = resourceManager->CreateFramebuffer(framebufferSize, ctx.allocationSettings.Framebuffer_ClearColor);
	ctx.lightingFramebuffer = resourceManager->CreateFramebuffer(framebufferSize, glm::vec4(ctx.allocationSettings.Lightmap_ClearValue), lightingPassFormat);

	rengine->createMappedBuffer(vk::BufferUsageFlagBits::eUniformBuffer, ctx.cameraBuffers);

	// section can be done in parrallel
	auto drawRenderpass = resourceManager->GetFramebuffer(ctx.drawFramebuffer)->renderpass;
	auto lightingFramebuffer = resourceManager->GetFramebuffer(ctx.lightingFramebuffer);

	{
		PROFILE_SCOPEN(colorPipeline);
		ctx.coloredQuadPipeline = make_unique<ColoredQuadPL>(rengine.get(), ctx.allocationSettings.ColoredQuad_MaxInstances);
		PipelineParameters prm;
		assetManager->LoadShaderFile("coloredQuad_vert.spv", prm.vertexSrc);
		assetManager->LoadShaderFile("coloredQuad_frag.spv", prm.fragmentSrc);
		prm.renderTarget = drawRenderpass;
		prm.cameraDB = ctx.cameraBuffers;
		ctx.coloredQuadPipeline->CreateGraphicsPipeline(prm);
	}
	{
		PROFILE_SCOPEN(texturePipeline);
		ctx.texturePipeline = make_unique<TexturedQuadPL>(rengine.get(), ctx.allocationSettings.TexturedQuad_MaxInstances);
		PipelineParameters prm;
		assetManager->LoadShaderFile("texture_vert.spv", prm.vertexSrc);
		assetManager->LoadShaderFile("texture_frag.spv", prm.fragmentSrc);
		prm.renderTarget = drawRenderpass;
		prm.cameraDB = ctx.cameraBuffers;
		std::array<int, 2> lightmapTextures = { lightingFramebuffer->textureIDs[0], lightingFramebuffer->textureIDs[1] };
		ctx.texturePipeline->CreateGraphicsPipeline(prm, &GlobalTextureDesc, true, lightmapTextures);
	}

	if (ctx.allocationSettings.AllocateLargeTileWorld) {

		assert(ctx.largeWorldMap != nullptr);

		{
			//PROFILE_SCOPEN(Allocate_tileworld);
			///*ctx.worldMap = make_unique<TileWorld>(rengine.get());*/
			//ctx.largeWorldMap->AllocateVulkanResources();
		}

		{
			PROFILE_SCOPEN(lightingPipeline);
			ctx.lightingPipeline = make_unique<LightingComputePL>(rengine.get());
			vector<uint8_t> comp, upscale, blur;
			assetManager->LoadShaderFile("lighting_comp.spv", comp);
			assetManager->LoadShaderFile("lightingUpscale_comp.spv", upscale);
			assetManager->LoadShaderFile("lightingBlur_comp.spv", blur);
			ctx.lightingPipeline->CreateComputePipeline(comp, upscale, blur, &ctx.largeWorldMap->deviceResources);
		}
		{
			PROFILE_SCOPEN(tilemapLightRasterPipeline);
			ctx.tilemapLightRasterPipeline = make_unique<TilemapLightRasterPL>(rengine.get());
			PipelineParameters prm;
			assetManager->LoadShaderFile("tilemapLightRaster_vert.spv", prm.vertexSrc);
			assetManager->LoadShaderFile("tilemapLightRaster_frag.spv", prm.fragmentSrc);
			prm.renderTarget = lightingFramebuffer->renderpass;
			prm.cameraDB = ctx.cameraBuffers;
			ctx.tilemapLightRasterPipeline->CreateGraphicsPipeline(prm, &GlobalTextureDesc, &ctx.largeWorldMap->deviceResources);
		}
		{
			PROFILE_SCOPEN(tilemapPipeline);
			ctx.tilemapPipeline = make_unique<TilemapPL>(rengine.get());
			PipelineParameters prm;
			assetManager->LoadShaderFile("tilemap_vert.spv", prm.vertexSrc);
			assetManager->LoadShaderFile("tilemap_frag.spv", prm.fragmentSrc);
			prm.renderTarget = drawRenderpass;
			prm.cameraDB = ctx.cameraBuffers;
			prm.flipFaces = false;
			ctx.tilemapPipeline->CreateGraphicsPipeline(prm, &GlobalTextureDesc, &ctx.largeWorldMap->deviceResources);
		}
	}
	{
		PROFILE_SCOPEN(textPipeline);
		ctx.textPipeline = make_unique<TextPL>(rengine.get(), ctx.allocationSettings.Text_MaxStrings, ctx.allocationSettings.Text_MaxStringLength);
		PipelineParameters prm;
		assetManager->LoadShaderFile("text_vert.spv", prm.vertexSrc);
		assetManager->LoadShaderFile("text_frag.spv", prm.fragmentSrc);
		prm.renderTarget = drawRenderpass;
		prm.cameraDB = ctx.cameraBuffers;
		ctx.textPipeline->CreateGraphicsPipeline(prm, &GlobalTextureDesc);
	}
	{
		PROFILE_SCOPEN(textPipeline);
		ctx.particlePipeline = make_unique<ParticleSystemPL>(rengine.get(), ctx.allocationSettings.ParticleSystem_MaxSmallSystems);
		PipelineParameters prm;
		assetManager->LoadShaderFile("particleSystem_vert.spv", prm.vertexSrc);
		assetManager->LoadShaderFile("particleSystem_frag.spv", prm.fragmentSrc);
		prm.renderTarget = drawRenderpass;
		prm.cameraDB = ctx.cameraBuffers;
		ctx.particlePipeline->CreateGraphicsPipeline(prm, computerParticleBuffer);
	}

	assert(ctx.allocationSettings.Worldspace_Background_DrawlistLayerCount == ctx.allocationSettings.Worldspace_Background_LayerAllocations.size());
	for (auto& setting : ctx.allocationSettings.Worldspace_Background_LayerAllocations) {
		ctx.backgroundDrawlistContexts.push_back(createDrawlistGraphicsContext(setting, ctx.cameraBuffers, drawRenderpass));
		ctx.backgroundDrawlistLayers.push_back(Drawlist(setting, assetManager.get()));
	}

	assert(ctx.allocationSettings.Worldspace_Foreground_DrawlistLayerCount == ctx.allocationSettings.Worldspace_Foreground_LayerAllocations.size());
	for (auto& setting : ctx.allocationSettings.Worldspace_Foreground_LayerAllocations) {
		ctx.foregroundDrawlistContexts.push_back(createDrawlistGraphicsContext(setting, ctx.cameraBuffers, drawRenderpass));
		ctx.foregroundDrawlistLayers.push_back(Drawlist(setting, assetManager.get()));
	}
}


std::unique_ptr<LargeTileWorld> Engine::CreateLargeTileWorld()
{
	auto tw = make_unique<LargeTileWorld>(rengine.get());
	tw->AllocateVulkanResources();
	return tw;
}

Engine::DrawlistGraphicsContext Engine::createDrawlistGraphicsContext(DrawlistAllocationConfiguration allocationSettings, MappedDoubleBuffer<coordinateTransformUBO_s> cameraBuffers, vk::RenderPass renderTarget) {

	DrawlistGraphicsContext ctx = DrawlistGraphicsContext(allocationSettings);

	{
		ctx.coloredQuadPipeline = make_unique<ColoredQuadPL>(rengine.get(), allocationSettings.ColoredQuad_MaxInstances);
		PipelineParameters prm;
		assetManager->LoadShaderFile("coloredQuad_vert.spv", prm.vertexSrc);
		assetManager->LoadShaderFile("coloredQuad_frag.spv", prm.fragmentSrc);
		prm.renderTarget = renderTarget;
		prm.cameraDB = cameraBuffers;
		ctx.coloredQuadPipeline->CreateGraphicsPipeline(prm);

	}
	{
		ctx.coloredTrianglesPipeline = make_unique<ColoredTrianglesPL>(rengine.get(), ctx.allocationSettings.ColoredTriangle_MaxInstances);
		PipelineParameters prm;
		assetManager->LoadShaderFile("coloredTriangles_vert.spv", prm.vertexSrc);
		assetManager->LoadShaderFile("coloredTriangles_frag.spv", prm.fragmentSrc);
		prm.renderTarget = renderTarget;
		prm.cameraDB = cameraBuffers;
		ctx.coloredTrianglesPipeline->CreateGraphicsPipeline(prm);
	}
	{
		ctx.texturedQuadPipeline = make_unique<TexturedQuadPL>(rengine.get(), allocationSettings.TexturedQuad_MaxInstances);
		PipelineParameters prm;
		assetManager->LoadShaderFile("texture_vert.spv", prm.vertexSrc);
		assetManager->LoadShaderFile("texture_frag.spv", prm.fragmentSrc);
		prm.renderTarget = renderTarget;
		prm.cameraDB = cameraBuffers;
		ctx.texturedQuadPipeline->CreateGraphicsPipeline(prm, &GlobalTextureDesc, false);
	}

	{
		ctx.textPipeline = make_unique<TextPL>(rengine.get(), allocationSettings.Text_MaxStrings, allocationSettings.Text_MaxStringLength);
		PipelineParameters prm;
		assetManager->LoadShaderFile("text_vert.spv", prm.vertexSrc);
		assetManager->LoadShaderFile("text_frag.spv", prm.fragmentSrc);
		prm.renderTarget = renderTarget;
		prm.cameraDB = cameraBuffers;
		ctx.textPipeline->CreateGraphicsPipeline(prm, &GlobalTextureDesc);
	}

	return ctx;
}

void Engine::Start(const VideoSettings& initialSettings, AssetManager::AssetPaths assetPaths, EngineMemoryAllocationConfiguration allocationSettings) {

	PROFILE_SCOPEN(Engine_Setup)

	{
		PROFILE_SCOPEN(Init_Window);

		rengine->initWindow(initialSettings.windowSetting, false);
		glfwSetWindowUserPointer(rengine->window, this);
		glfwSetFramebufferSizeCallback(rengine->window, framebufferResizeCallback);
		glfwSetKeyCallback(rengine->window, KeyCallback);
		glfwSetMouseButtonCallback(rengine->window, mouseButtonCallback);
		glfwSetScrollCallback(rengine->window, scroll_callback);
	}

	input = make_unique<Input>(rengine->window);
	Behaviour::input = input.get();

	DebugLog("Initialized Window");

	{
		PROFILE_SCOPEN(Setup_Vulkan);

		rengine->initVulkan(initialSettings.swapChainSetting, 1);
		rengine->createCommandPools();
		rengine->createDescriptorPool();
		rengine->createCommandBuffers();
		rengine->createTextureSamplers();
	}

	resourceManager = std::make_unique<ResourceManager>(rengine.get(), resourceChangeFlags.get());
	assetManager = std::make_unique<AssetManager>(rengine.get(), assetPaths, resourceManager.get());


	DebugLog("Initialized Vulkan");

#ifdef TRACY_ENABLE
	rengine->initTracyContext();
	DebugLog("Initialized Tracy");
#endif

	this->winW = initialSettings.windowSetting.windowSizeX;
	this->winH = initialSettings.windowSetting.windowSizeY;
	lastwinW = winW;
	lastwinH = winH;

	// create some buffers
	{
		//screenSpaceTransformUploader.CreateBuffers(rengine.get());
		rengine->createMappedBuffer(vk::BufferUsageFlagBits::eUniformBuffer, screenSpaceTransformCameraBuffer);
		AllocateQuad(rengine.get(), quadMeshBuffer);

		// allocated dedicated device memory for compute driven particle systems
		{
			// todo: use less ambigous size. ParticleGroup_larg is not labled as being device only
			rengine->CreateDeviceOnlyStorageBuffer(sizeof(Particle) * MAX_PARTICLES_LARGE * MAX_PARTICLE_SYSTEMS_LARGE, false, computerParticleBuffer);

			//computerParticleBuffer.size = sizeof(ParticleSystemPL::device_particle_ssbo);

			//rengine->createBuffer(
			//	computerParticleBuffer.size,
			//	vk::BufferUsageFlagBits::eStorageBuffer, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			//	computerParticleBuffer.buffer,
			//	computerParticleBuffer.allocation,
			//	true);
		}
	}

	InitImgui(); // imgui needs to be initialized to register textures with it

	{
		GlobalTextureDesc.CreateLayout(0, max_bindless_resources);
		GlobalTextureDesc.CreateDescriptors();
	}

	// this will not be scene agnostic once tilmap particle collisions are added
	{
		particleComputePipeline = make_unique<ParticleComputePL>(rengine.get());
		vector<uint8_t> comp;
		assetManager->LoadShaderFile("particleSystem_comp.spv", comp);
		particleComputePipeline->CreateComputePipeline(comp, computerParticleBuffer);
	}


	assert(allocationSettings.Screenspace_DrawlistLayerCount == allocationSettings.Screenspace_DrawlistLayerAllocations.size());
	for (auto& setting : allocationSettings.Screenspace_DrawlistLayerAllocations) {
		screenspaceDrawlistContexts.push_back(createDrawlistGraphicsContext(setting, screenSpaceTransformCameraBuffer, rengine->swapchainRenderPass));
		screenspaceDrawlistLayers.push_back(Drawlist(setting, assetManager.get()));
	}


	vector<uint8_t> checkerData;
	genCheckerboard(400, vec4(1, 1, 0, 1), vec4(0, 0, 1, 1), 6, checkerData);
	assetManager->CreateDefaultSprite(400, 400, checkerData);

	DebugLog("Initialized pipelines");
}

void Engine::ShowWindow() {
	glfwShowWindow(rengine->window);
	_onWindowResize();
}

void Engine::ApplyNewVideoSettings(const VideoSettings settings) {
	newVideoSettingsRequested = true;
	requestedSettings = settings;
}

void Engine::recordSceneContextGraphics(const SceneGraphicsContext& ctx, std::shared_ptr<Scene> scene, const Camera& camera, vk::CommandBuffer& cmdBuffer) {

	auto fb = resourceManager->GetFramebuffer(ctx.drawFramebuffer);
	auto lightingPass_fb = resourceManager->GetFramebuffer(ctx.lightingFramebuffer);

	// update camera transform for scene
	{
		ctx.cameraBuffers.buffersMapped[rengine->currentFrame]->aspectRatio = (float)fb->extents[rengine->currentFrame].height / (float)fb->extents[rengine->currentFrame].width;
		ctx.cameraBuffers.buffersMapped[rengine->currentFrame]->position = -camera.position;
		ctx.cameraBuffers.buffersMapped[rengine->currentFrame]->zoom = camera.zoom;
	}

	// viewport for camera
	{
		// invert defualt vulkan coordinates. Set bottom of screen to 0 and top of screen to 1
		vk::Viewport viewport;
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(fb->extents[rengine->currentFrame].height);
		viewport.width = static_cast<float>(fb->extents[rengine->currentFrame].width);
		viewport.height = -static_cast<float>(fb->extents[rengine->currentFrame].height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		cmdBuffer.setViewport(0, 1, &viewport);

		vk::Rect2D scissor;
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = fb->extents[rengine->currentFrame];
		cmdBuffer.setScissor(0, 1, &scissor);
	}

	// bind global quad mesh
	{
		vk::Buffer vertexBuffers[] = { quadMeshBuffer.vertexBuffer };
		vk::DeviceSize offsets[] = { 0 };
		cmdBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
		cmdBuffer.bindIndexBuffer(quadMeshBuffer.indexBuffer, 0, vk::IndexType::eUint16);
	}


	// tile map lighting pass
	if (ctx.tilemapPipeline != nullptr)
	{
		ZoneScopedN("tilemap PL");
		rengine->beginRenderpass(lightingPass_fb, cmdBuffer);
		ctx.tilemapLightRasterPipeline->recordCommandBuffer(cmdBuffer, ctx.tilemapTextureAtlas, ctx.largeWorldMap->lightingSettings);
		cmdBuffer.endRenderPass();
		rengine->insertFramebufferTransitionBarrier(cmdBuffer, lightingPass_fb);
	}

	rengine->beginRenderpass(fb, cmdBuffer);

	// render background drawlists
	for (size_t i = 0; i < ctx.backgroundDrawlistContexts.size(); i++)
		recordDrawlistContextGraphics(ctx.backgroundDrawlistContexts[i], ctx.backgroundDrawlistLayers[i], cmdBuffer);

	// tilemap
	if (ctx.tilemapPipeline != nullptr)
	{
		ZoneScopedN("tilemap PL");
		ctx.tilemapPipeline->recordCommandBuffer(cmdBuffer, ctx.tilemapTextureAtlas, lightingPass_fb->textureIDs[rengine->currentFrame]);
	}

	// Textured quad
	{
		ZoneScopedN("Textured quad PL");

		if (scene->sceneData.spriteRenderers.size() > 0) {
			vector<TexturedQuadPL::ssboObjectInstanceData> drawlist;
			drawlist.reserve(scene->sceneData.spriteRenderers.size());

			TexturedQuadPL::ssboObjectInstanceData* GPUBuffer = ctx.texturePipeline->getUploadMappedBuffer();
			int instanceIndex = 0;

			assert(scene->sceneData.spriteRenderers.size() < ctx.allocationSettings.TexturedQuad_MaxInstances);
			for (auto& [entID, renderer] : scene->sceneData.spriteRenderers)
			{
				Entity* entity = renderer._entityCache;

				Sprite* s = renderer._spriteCache;

				auto drawObject = GPUBuffer + instanceIndex;

				drawObject->uvMin = vec2(0.0f);
				drawObject->uvMax = vec2(1.0f);
				drawObject->useLightMap = renderer.useLightMap;
				drawObject->tex = s->textureID;

				if (entity->HasParent()) {
					Transform global = entity->GetGlobalTransform();
					drawObject->translation = global.position;
					drawObject->scale = global.scale;
					drawObject->rotation = global.rotation;
				}
				else {
					drawObject->translation = entity->transform.position;
					drawObject->scale = entity->transform.scale;
					drawObject->rotation = entity->transform.rotation;
				}

				if (s->atlas.size() > 0) {
					auto atEntry = s->atlas[renderer.atlasIndex];
					drawObject->uvMin = atEntry.uv_min;
					drawObject->uvMax = atEntry.uv_max;
				}

				instanceIndex++;
			}


			ctx.texturePipeline->recordCommandBuffer(cmdBuffer, instanceIndex);
			runningStats.sprite_render_count += instanceIndex;
		}
	}

	// colored quad
	if (scene->sceneData.colorRenderers.size() > 0)
	{
		ZoneScopedN("Colored quad PL");

		//vector<ColoredQuadPL::InstanceBufferData> drawlist;
		//drawlist.reserve(scene->sceneData.colorRenderers.size());

		auto buffer = ctx.coloredQuadPipeline->getUploadMappedBuffer();

		int bufferIndex = 0;
		for (auto& renderer : scene->sceneData.colorRenderers)
		{
			const auto& entity = scene->sceneData.entities.at(renderer.first);

			ColoredQuadPL::InstanceBufferData& instanceData = buffer[bufferIndex++];
			instanceData.color = renderer.second.color;
			instanceData.circle = renderer.second.shape == ColorRenderer::Shape::Circle;

			if (entity.HasParent()) {
				Transform global = entity.GetGlobalTransform();
				instanceData.position = global.position;
				instanceData.scale = global.scale;
				instanceData.rotation = global.rotation;
			}
			else {
				instanceData.position = entity.transform.position;
				instanceData.scale = entity.transform.scale;
				instanceData.rotation = entity.transform.rotation;
			}

			//drawlist.push_back(instanceData);
		}

		//ctx.colorPipeline->UploadInstanceData(drawlist);


		ctx.coloredQuadPipeline->recordCommandBuffer(cmdBuffer, bufferIndex);
	}

	// particles
	{
		float particleDeltaTime = scene->paused ? 0.0f : deltaTime;

		int smallSystemIndex = 0;
		std::vector<int> indexes;
		std::vector<int> systemSizes;
		std::vector<int> particleCounts;
		for (auto& [entID, renderer] : scene->sceneData.particleSystemRenderers)
		{
			const auto& entity = scene->sceneData.entities.at(entID);

			if (renderer.size == ParticleSystemRenderer::Size::Small) {

				if (entity.HasParent()) {
					Transform global = entity.GetGlobalTransform();
					renderer.runSimulation(particleDeltaTime, global.position);
				}
				else {
					renderer.runSimulation(particleDeltaTime, entity.transform.position);
				}

				ctx.particlePipeline->UploadInstanceData(*renderer.hostParticleBuffer.get(), smallSystemIndex);

				indexes.push_back(smallSystemIndex++);
				systemSizes.push_back(0);
				particleCounts.push_back(renderer.configuration.particleCount);
			}
			else if (renderer.size == ParticleSystemRenderer::Size::Large) {
				assert(renderer.token != nullptr);
				indexes.push_back(renderer.token->index);
				systemSizes.push_back(1);
				particleCounts.push_back(renderer.configuration.particleCount);
			}
			else {
				assert(false);
			}

		}

		ctx.particlePipeline->recordCommandBuffer(cmdBuffer, indexes, systemSizes, particleCounts);
	}

	// text
	{
		ZoneScopedN("Text PL");

		if (scene->sceneData.textRenderers.size() > 0) {
			ctx.textPipeline->ClearTextData(rengine->currentFrame);

			int i = 0;
			for (auto& [entID, r] : scene->sceneData.textRenderers) {

				const auto& entity = scene->sceneData.entities.at(entID);

				Font* f = assetManager->GetFont(r.font);
				Sprite* sprite = assetManager->GetSprite(f->atlas);

				if (r.dirty) {
					r.quads.clear();
					r.quads.resize(r.text.length());
					CalculateQuads(f, r.text, r.quads.data());
					r.dirty = false;
				}

				TextPL::textHeader header;
				header.color = r.color;
				header.textLength = glm::min(ctx.allocationSettings.Text_MaxStringLength, (uint32_t)r.quads.size());
				header._textureIndex = sprite->textureID;

				if (entity.HasParent()) {
					Transform global = entity.GetGlobalTransform();
					header.position = global.position;
					header.scale = global.scale;
					header.rotation = global.rotation;
				}
				else {
					header.position = entity.transform.position;
					header.scale = entity.transform.scale;
					header.rotation = entity.transform.rotation;
				}

				//TODO: could potentially assume this data is already here if the renderer is not dirty ?
				//TextPL::textObject textData;
				//std::copy(r.quads.begin(), r.quads.end(), textData.quads);

				ctx.textPipeline->UploadTextData(rengine->currentFrame, i, header, r.font, r.quads);
				i++;
			}
			ctx.textPipeline->recordCommandBuffer(cmdBuffer);
		}
	}

	// render foreground drawlists
	for (size_t i = 0; i < ctx.foregroundDrawlistContexts.size(); i++)
		recordDrawlistContextGraphics(ctx.foregroundDrawlistContexts[i], ctx.foregroundDrawlistLayers[i], cmdBuffer);

	cmdBuffer.endRenderPass();

	rengine->insertFramebufferTransitionBarrier(cmdBuffer, fb);
}

void Engine::recordDrawlistContextGraphics(const DrawlistGraphicsContext& ctx, const Drawlist& drawData, vk::CommandBuffer cmdBuffer) {

	// screenspace texture
	if (drawData.texturedQuadInstanceIndex > 0)
	{
		auto buffer = ctx.texturedQuadPipeline->getUploadMappedBuffer();

		// slap on the fb textures here
		for (size_t i = 0; i < drawData.framebufferDrawData.size(); i++)
		{
			auto item = drawData.framebufferDrawData[i];
			auto fb = resourceManager->GetFramebuffer(item.fb);

			float w = fb->extents[rengine->currentFrame].width;
			float h = fb->extents[rengine->currentFrame].height;

			//buffer[i].uvMin = vec2(0);
			//buffer[i].uvMax = vec2(1);
			buffer[i].uvMin = vec2(0, 1);
			buffer[i].uvMax = vec2(1, 0);
			buffer[i].translation = item.pos;
			buffer[i].scale = glm::vec2((w / h) * item.height, item.height);
			buffer[i].rotation = item.rotation;
			buffer[i].tex = fb->textureIDs[rengine->currentFrame];

		}

		std::copy(drawData.texturedQuadInstanceData.begin(), drawData.texturedQuadInstanceData.begin() + drawData.texturedQuadInstanceIndex, buffer + drawData.framebufferDrawData.size());

		ctx.texturedQuadPipeline->recordCommandBuffer(cmdBuffer, drawData.texturedQuadInstanceIndex + drawData.framebufferDrawData.size());
	}

	// screenspace quad
	if (drawData.coloredQuadInstanceIndex > 0)
	{
		auto buffer = ctx.coloredQuadPipeline->getUploadMappedBuffer();
		std::copy(drawData.coloredQuadInstanceData.begin(), drawData.coloredQuadInstanceData.begin() + drawData.coloredQuadInstanceIndex, buffer);

		ctx.coloredQuadPipeline->recordCommandBuffer(cmdBuffer, drawData.coloredQuadInstanceIndex);
	}

	// triangles
	if(drawData.coloredQuadInstanceIndex > 0)
	{
		auto colorBuffer = ctx.coloredTrianglesPipeline->GetColorMappedBuffer(rengine->currentFrame);
		auto vertexBuffer = ctx.coloredTrianglesPipeline->GetVertexMappedBuffer(rengine->currentFrame);
		std::copy(drawData.coloredTriangleColorData.begin(), drawData.coloredTriangleColorData.begin() + drawData.coloredTrianglesInstanceIndex, colorBuffer);
		std::copy(drawData.coloredTriangleVertexData.begin(), drawData.coloredTriangleVertexData.begin() + drawData.coloredTrianglesInstanceIndex * ColoredTrianglesPL::verticesPerMesh, vertexBuffer);

		ctx.coloredTrianglesPipeline->recordCommandBuffer(cmdBuffer, drawData.coloredTrianglesInstanceIndex);
	}
	bindQuadMesh(cmdBuffer);

	// screenspace text
	{
		int memSlot = 0;
		{
			ctx.textPipeline->ClearTextData(rengine->currentFrame);
			for (int i = 0; i < drawData.textInstanceIndex; i++)
			{
				const Drawlist::screenSpaceTextDrawItem& item = drawData.textInstanceData[i];
				Font* f = assetManager->GetFont(item.font);
				auto sprite = assetManager->GetSprite(f->atlas);

				//TextPL::textObject textData;
				vector<charQuad> quads;
				quads.resize(item.text.length());
				//CalculateQuads(f, item.text, textData.quads);
				CalculateQuads(f, item.text, quads.data());

				TextPL::textHeader header = item.header;
				header.scale = vec2(f->fontHeight * 2) * item.scaleFactor;
				header._textureIndex = sprite->textureID;

				ctx.textPipeline->UploadTextData(rengine->currentFrame, memSlot++, item.header, item.font, quads);
			}
		}
		ctx.textPipeline->recordCommandBuffer(cmdBuffer);
	}
}

bool Engine::QueueNextFrame(const std::vector<SceneRenderJob>& sceneRenderJobs, bool drawImgui) {
	ZoneScopedN("queue next frame");

	double time = glfwGetTime();
	deltaTime = time - lastTime;
	//paused_deltaTime = deltaTime;
	framerate = 1.0 / deltaTime;
	frameTimes[frameTimeIndex++] = framerate;
	frameTimeIndex = frameTimeIndex % frameTimeBufferCount;

	windowResizedLastFrame = false;

	lastTime = time;
	this->time = time;

	runningStats.sprite_render_count = 0;

	// compute buffers can be recoreded in parallel

	// this can be simplified logically
	{
		ZoneScopedN("Update physics");
		for (auto& ctx : sceneRenderJobs) {
			if (ctx.scene->paused)
				continue;

			ctx.scene->physicsTimer += deltaTime;

			while (ctx.scene->physicsTimer >= timeStep) {
				ctx.scene->bworld.Step(timeStep, velocityIterations, positionIterations);
				ctx.scene->physicsTimer -= timeStep;
			}

			for (auto& body : ctx.scene->sceneData.rigidbodies)
			{
				ctx.scene->sceneData.entities.at(body.first).transform.position = body.second._getPosition();
				ctx.scene->sceneData.entities.at(body.first).transform.rotation = body.second._getRotation();
			}
		}
	}


	// wait for the this frame's compute command buffer to become available
	rengine->WaitForComputeSubmission();
	rengine->WaitForComputeCompletion();


	// TODO: move block to new "recordSceneComputeContext" function. Doesn't have to have the same parameters as 
	// the graphics context function, but it should still be sectioned out to a function

	// record compute command buffer without submmiting it
	{
		ZoneScopedN("Record compute shaders");
		auto computeCmdBuffer = rengine->getNextComputeCommandBuffer();

		// lighting compute
		for (auto& job : sceneRenderJobs) {
			//const auto& ctx = sceneRenderContextMap.find(job.sceneRenderCtxID)->second.pl; 
			const auto& ctx = sceneRenderContextMap.at(job.sceneRenderCtxID);
			if (ctx.largeWorldMap == nullptr)
				continue;

			// transfer tiles to read-only memory before starting renderpass
			{
				ZoneScopedN("chunk updates");
				ctx.largeWorldMap->stageChunkUpdates(computeCmdBuffer);
			}

			auto baseBightingtUpdates = ctx.largeWorldMap->getBaseLightingUpdateData();
			auto blurBightingtUpdates = ctx.largeWorldMap->getBlurLightingUpdateData();
			ctx.lightingPipeline->stageLightingUpdate(baseBightingtUpdates, blurBightingtUpdates);
			ctx.lightingPipeline->recordCommandBuffer(computeCmdBuffer, baseBightingtUpdates.size(), blurBightingtUpdates.size(), ctx.largeWorldMap->lightingSettings);
			ctx.largeWorldMap->baseChunkLightingJobs.clear();
			ctx.largeWorldMap->blurChunkLightingJobs.clear();
		}

		// record particle compute for every particle system in every scene
		{
			vector<ParticleComputePL::DispatchInfo> dispachInfos;


			for (auto& job : sceneRenderJobs)
			{
				float particleDeltaTime = job.scene->paused ? 0.0f : deltaTime;

				for (auto& [entID, renderer] : job.scene->sceneData.particleSystemRenderers) {
					if (renderer.computeContextDirty) {

						if (renderer.size == ParticleSystemRenderer::Size::Large && renderer.token == nullptr) {
							int selectedIndex = -1;
							for (size_t i = 0; i < particleSystemResourceTokens.size(); i++)
							{
								if (particleSystemResourceTokens[i].active == false) {
									selectedIndex = i;
									break;
								}
							}
							assert(selectedIndex != -1);

							renderer.token = &particleSystemResourceTokens[selectedIndex];
							renderer.token->index = selectedIndex;
							renderer.token->active = true;
						}
					}

					if (renderer.size == ParticleSystemRenderer::Size::Large) {

						assert(renderer.token != nullptr);

						const auto& entity = job.scene->sceneData.entities.at(entID);

						renderer.spawntimer += particleDeltaTime;
						float step = 1.0f / renderer.configuration.spawnRate;
						int particlesToSpawn = static_cast<int>(renderer.spawntimer / step);
						renderer.spawntimer -= particlesToSpawn * step;

						ParticleComputePL::DispatchInfo info{
							.systemIndex = renderer.token->index,
							.particleCount = renderer.configuration.particleCount,
							.particlesToSpawn = particlesToSpawn,
							.init = renderer.computeContextDirty,
							//.spawnPosition = entity.transform.position
						};

						if (entity.HasParent()) {
							mat4 m = entity.GetLocalToGlobalMatrix();
							info.spawnPosition = extractPosition(m);
						}
						else {
							info.spawnPosition = entity.transform.position;
						}

						dispachInfos.push_back(info);

						// TODO: only update if dirty?
						//renderer.configuration.burstMode = 0;

						//renderer.configuration.particleCount = 0;
						//renderer.configuration.burstMode = 0;
						//renderer.configuration.burstRepeat = 0;
						//renderer.configuration.spawnRate = 0; 
						//renderer.configuration.particleLifeSpan =0; 
						//renderer.configuration.gravity = 0;
						//renderer.configuration.startSize = 0;
						//renderer.configuration.endSize = 0;
						//renderer.configuration.startColor = {0, 0, 0, 0};
						//renderer.configuration.endColor = { 0, 0, 0, 0 };

						particleComputePipeline->UploadInstanceConfigurationData(renderer.configuration, renderer.token->index);
					}

					renderer.computeContextDirty = false;
				}
			}
			if (dispachInfos.size() > 0)
				particleComputePipeline->RecordCommandBuffer(computeCmdBuffer, deltaTime, dispachInfos);
		}

		TracyVkCollect(rengine->tracyComputeContexts[rengine->currentFrame], rengine->computeCommandBuffers[rengine->currentFrame]);

		computeCmdBuffer.end();
	}

	rengine->QueueComputeSubmission();


	rengine->WaitForGraphicsSubmission();
	rengine->WaitForGraphicsCompletion();

	resourceManager->ReleaseFreeableTextures();

	resourceManager->HandleFramebufferRecreation();


	//{
	//	// free up indexes in bindless descriptor set
	//	while (textureBindingDeletionQueue.size() > 0) {
	//		auto& id = textureBindingDeletionQueue.front();
	//		textureBindingDeletionQueue.pop();
	//		if (globalTextureBindingManager.HasBinding(id)) // may have been deleted before it was finished uploading
	//			globalTextureBindingManager.RemoveBinding(id);
	//	}

	//	bool texturesAdded = resourceChangeFlags->TexturesAdded();
	//	if (texturesAdded) {
	//		globalTextureBindingManager.InvalidateDescriptors();
	//	}

	//	// mainly needed for asynchronous textures becoming available
	//	if (texturesAdded) {
	//		for (auto& [ID, tex] : *resourceManager->GetInternalTextureResources()) {
	//			globalTextureBindingManager.AddBinding(ID, (Texture*)&tex);
	//		}
	//	}

	//	if (resourceChangeFlags->TextureFiltersChanged())
	//		globalTextureBindingManager.InvalidateDescriptors();

	//	if (globalTextureBindingManager.IsDescriptorDirty(rengine->currentFrame) == true) {

	//		std::vector<int> indexes;
	//		std::vector <Texture*> textures;

	//		// TODO: Optimize use of bindless descriptors by only adding new textures to descriptor instead of all loaded textures.
	//		// Will need additional queues for each frame in flight to keep track of what textures have been fully uploaded and need to be bound.
	//		{
	//			std::unique_lock<std::mutex> lock(resourceManager->texMapMtx);
	//			for (auto& [ID, tex] : *resourceManager->GetInternalTextureResources()) {

	//				if (globalTextureBindingManager.HasBinding(ID)) {

	//					indexes.push_back(globalTextureBindingManager.getIndexFromBinding(ID));
	//					auto tt = globalTextureBindingManager.getValueFromBinding(ID);
	//					textures.push_back(tt);
	//				}
	//			}
	//		}

	//		GlobalTextureDesc.AddDescriptors(indexes, textures, rengine->currentFrame);
	//		globalTextureBindingManager.ClearDescriptorDirty(rengine->currentFrame);
	//	}

	//	// use flag for thread safety
	//	if (texturesAdded)
	//		resourceChangeFlags->ClearTexturesAdded();

	//	resourceChangeFlags->ClearTextureFilterschanged();
	//}

	// very slow
	{
		bool texturesAdded = resourceChangeFlags->TexturesAdded();
		bool filtersChanged = resourceChangeFlags->TextureFiltersChanged();

		if (texturesAdded || filtersChanged) {
			textureDescriptorDirtyFlags = { true, true };
		}
		resourceChangeFlags->ClearTexturesAdded();
		resourceChangeFlags->ClearTextureFilterschanged();

		if (textureDescriptorDirtyFlags[rengine->currentFrame])
		{
			std::vector<int> indexes;
			std::vector <const Texture*> textures;

			// TODO: Optimize use of bindless descriptors by only adding new textures to descriptor instead of all loaded textures.
			// Will need additional queues for each frame in flight to keep track of what textures have been fully uploaded and need to be bound.
			// Extra consideration needed for async uploading
			{
				std::unique_lock<std::mutex> lock(resourceManager->texMapMtx);
				auto& srcMap = *resourceManager->GetInternalTextureResources();
				indexes.reserve(srcMap.size() + 1);
				textures.reserve(srcMap.size() + 1);

				for (auto& [ID, tex] : srcMap) {

					indexes.push_back(ID);
					textures.push_back(&tex);
				}
			}

			GlobalTextureDesc.AddDescriptors(indexes, textures, rengine->currentFrame);

			textureDescriptorDirtyFlags[rengine->currentFrame] = false;
		}
	}

	auto cmdBuffer = rengine->getNextGfxCommandBuffer();

	for (auto& job : sceneRenderJobs)
		recordSceneContextGraphics(sceneRenderContextMap.at(job.sceneRenderCtxID), job.scene, job.camera, cmdBuffer);

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
			//resourceManager->ResizeFramebuffer(tmp_cameraFB, { winW, winH });

			_onWindowResize();
			windowResizedLastFrame = true;
		}


		// then wait for the frame to be presented
		imageIndex = rengine->WaitForSwapChainImageAvailableAndHandleWindowChanges((newVideoSettingsRequested ? &requestedSettings.windowSetting : nullptr));
	}

	rengine->beginSwapchainRenderpass(imageIndex, cmdBuffer, vec4(0.0, 0.0, 0.0, 1.0));

	// screen space transformer
	{
		coordinateTransformUBO_s* transform = screenSpaceTransformCameraBuffer.buffersMapped[rengine->currentFrame];

		transform->aspectRatio = (float)winH / (float)winW;
		transform->position = vec2(-(float)winW / 2.0f, -(float)winH / 2.0f);
		transform->zoom = 2.0f / (float)winH;
	}

	// swap chain full screen viewport for screenspace
	{
		vk::Viewport viewport;
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(rengine->swapChainExtent.height);
		//viewport.y = 800.0f;
		viewport.width = static_cast<float>(rengine->swapChainExtent.width);
		viewport.height = -static_cast<float>(rengine->swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		cmdBuffer.setViewport(0, 1, &viewport);

		vk::Rect2D scissor;
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = rengine->swapChainExtent;
		cmdBuffer.setScissor(0, 1, &scissor);
	}

	bindQuadMesh(cmdBuffer);

	for (auto& ctx : sceneRenderJobs)
	{
		runningStats.entity_count = ctx.scene->sceneData.entities.size();
	}

	for (size_t i = 0; i < screenspaceDrawlistContexts.size(); i++)
		recordDrawlistContextGraphics(screenspaceDrawlistContexts[i], screenspaceDrawlistLayers[i], cmdBuffer);

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

	for (auto& list : screenspaceDrawlistLayers)
		list.ResetIndexes();
	for (auto& job : sceneRenderJobs)
	{
		auto& ctx = sceneRenderContextMap.at(job.sceneRenderCtxID);
		for (auto& ls : ctx.backgroundDrawlistLayers)
			ls.ResetIndexes();
		for (auto& ls : ctx.foregroundDrawlistLayers)
			ls.ResetIndexes();
	}

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
	init_info.RenderPass = rengine->swapchainRenderPass;
	init_info.Queue = rengine->devContext.graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = rengine->swapChainImages.size();
	init_info.ImageCount = rengine->swapChainImages.size();
	init_info.Subpass = 0;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);


	//execute a gpu command to upload imgui font textures
	auto cmdBuffer = rengine->beginSingleTimeCommands_gfx();
	//ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
	rengine->endSingleTimeCommands_gfx(cmdBuffer);

	//clear font textures from cpu data
	//ImGui_ImplVulkan_DestroyFontUploadObjects();




	//add the destroy the imgui created structures
	//_mainDeletionQueue.push_function([=]() {

	//	vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	//	ImGui_ImplVulkan_Shutdown();
	//	});


}