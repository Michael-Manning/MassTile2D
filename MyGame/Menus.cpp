#include "stdafx.h"

#include <string>

#include <glm/glm.hpp>

#include "Utils.h"
#include "Menus.h"

using namespace std;
using namespace glm;

namespace {
	const vec2 btnSize(60, 30);
	bool Button(vec2 pos, Input* input, vec2 size = btnSize ) {
		if (input->getMouseBtnDown(MouseBtn::Left) == false)
			return false;
		return within(pos, pos + size, input->getMousePos());

	}

	float updateTimer = 0;
	float frameRateStat = 0;

	const char* options[] = { "windowed", "borderless", "exclusive fullscreen" };

}

void DoSettingsMenu(MenuState& state, Engine* engine) {
	auto white = vec4(1.0);
	//engine.addScreenSpaceQuad(vec4(1.0), input->getMousePos(), vec2(50));

	engine->addScreenSpaceText(state.smallfont, { 0, 0 }, white, "fps: %d", (int)frameRateStat);

	engine->addScreenSpaceText(state.bigfont, { 200, 100 }, white, "Settings");

	const auto optionA = "fullscreen";
	engine->addScreenSpaceText(state.medfont, { 200, 300 }, white, "Window mode: %s", options[(int)state.selectedWindowOption]);

	engine->addScreenSpaceCenteredQuad(white, vec2(900, 320) + btnSize / 2.0f, btnSize);
	if (Button({ 900, 320 }, state.input)) {
		state.selectedWindowOption = (WindowMode)(((int)state.selectedWindowOption + 1) % 3);
	}

	if (engine->time - updateTimer > 0.2f) {
		frameRateStat = engine->_getAverageFramerate();
		updateTimer = engine->time;
	}


	//	engine.addScreenSpaceQuad(vec4(0.7), vec2(250, 527), vec2(100, 40));
	vec4 tc = vec4(1.0);
	if (within(vec2(250, 527) - vec2(100, 40) / 2.0f, vec2(250, 527) + vec2(100, 40) / 2.0f, state.input->getMousePos())) {
		tc = vec4(0.3, 0.9, 0.3, 1.0);
		if (state.input->getMouseBtnDown(MouseBtn::Left)) {
			state.videoSettings->windowSetting.windowMode = state.selectedWindowOption;
			engine->ApplyNewVideoSettings(*state.videoSettings);
		}
	}
	engine->addScreenSpaceText(state.medfont, { 200, 500 }, tc, "Apply");
}
