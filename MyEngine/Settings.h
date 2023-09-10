#pragma once


enum class WindowMode {
	Windowed,
	Borderless,
	Fullscreen
};

struct SwapChainSetting {
	bool vsync = false;
	bool capFramerate = false;
};

struct VideoSettings {
	int resolutionX;
	int resolutionY;
	WindowMode windowMode;
	SwapChainSetting swapChainSetting;
};


