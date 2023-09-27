#pragma once

#include <string>

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


