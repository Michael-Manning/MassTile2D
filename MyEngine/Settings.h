#pragma once

#include <string>
#include <stdint.h>
#include <vector>

#include <glm/glm.hpp>

// Video settings notes:
// resolution and window size are independent because the engine will support rendering at a downscaled resolution
// and then upscaling. This is only applicable in fullscreen mode. Otherwise, the resolution and window size are synchronized.

enum class WindowMode {
	Windowed = 0,
	Borderless,
	Fullscreen,
	WindowModeCount
};

struct SwapChainSetting {
	bool vsync = false;
	bool capFramerate = false;
	int resolutionX;
	int resolutionY;
};

struct WindowSetting {
	WindowMode windowMode;
	int windowSizeX = 0;
	int windowSizeY = 0;
	std::string name;
};

struct VideoSettings {
	WindowSetting windowSetting;
	SwapChainSetting swapChainSetting;
};

// populated with some common sense default values for reference

struct DrawlistAllocationConfiguration {
	uint32_t ColoredQuad_MaxInstances = 1024;
	uint32_t ColoredTriangle_MaxInstances = 1024;
	uint32_t TexturedQuad_MaxInstances = 1024;
	uint32_t Text_MaxStrings = 32;
	uint32_t Text_MaxStringLength = 128;
};

struct SceneGraphicsAllocationConfiguration {
	uint32_t ColoredQuad_MaxInstances = 2048;
	uint32_t TexturedQuad_MaxInstances = 2048;
	uint32_t Text_MaxStrings = 128;
	uint32_t Text_MaxStringLength = 128;

	uint32_t ParticleSystem_MaxSmallSystems = 20;
	uint32_t ParticleSystem_MaxSmallSystemParticles = 200; 

	glm::vec4 Framebuffer_ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	bool transparentFramebufferBlending = false;

	bool AllocateTileWorld = true;
	float Lightmap_ClearValue = 0.0f;
};

struct EngineMemoryAllocationConfiguration {

	//// layers which appear behind scene graphics components
	//int Worldspace_Background_DrawlistLayerCount = 1;
	//std::vector<DrawlistAllocationConfiguration> Worldspace_Background_LayerAllocations;

	//// layers which appear in front of scene graphics components
	//int Worldspace_Forground_DrawlistLayerCount = 1;
	//std::vector<DrawlistAllocationConfiguration> Worldspace_Foreground_LayerAllocations;

	uint32_t ParticleSystem_MaxLargeSystems = 4;
	uint32_t ParticleSystem_MaxLargeSystemParticles = 100000;

	int Screenspace_DrawlistLayerCount = 1;
	std::vector<DrawlistAllocationConfiguration> DrawlistLayerAllocations;
};
