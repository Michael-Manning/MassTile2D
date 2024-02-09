#pragma once

#include <memory>

#include "Engine.h"
#include "Input.h"
#include "Settings.h"

struct UIState {

	std::shared_ptr<Input> input;

	VideoSettings* videoSettings;

	WindowMode selectedWindowOption;

	fontID bigfont;
	fontID medfont;
	fontID smallfont;

	enum class Page {
		None,
		Main,
		Settings,
		VideoSettings,
		AudioSettings,
	};

	Page currentPage = Page::Main;
};

// capable of changing and applying video/audio settings via engine
void DoSettingsMenu(UIState& state, Engine* engine);