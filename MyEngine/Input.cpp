#include "stdafx.h"

#include <GLFW/glfw3.h>
#include <cassert>
#include <unordered_map>
#include <glm/glm.hpp>

#include "Input.h"

using enum KeyCode;

namespace {
	static std::unordered_map<KeyCode, int> keyInputMap = {
			{LeftControl, GLFW_KEY_LEFT_CONTROL},
			{RightControl, GLFW_KEY_RIGHT_CONTROL},
			{LeftArrow, GLFW_KEY_LEFT},
			{RightArrow, GLFW_KEY_RIGHT},
			{UpArrow, GLFW_KEY_UP},
			{DownArrow, GLFW_KEY_DOWN},
	};

	static std::unordered_map<MouseBtn, int> mouseInputMap = {
		{MouseBtn::Left, GLFW_MOUSE_BUTTON_LEFT},
		{MouseBtn::Right, GLFW_MOUSE_BUTTON_RIGHT},
	};

	int AsciiToGLFWKey(char c) {
		// Check if character is an uppercase letter
		if (c >= 'A' && c <= 'Z') {
			return GLFW_KEY_A + (c - 'A');
		}

		// Check if character is a lowercase letter
		if (c >= 'a' && c <= 'z') {
			return GLFW_KEY_A + (c - 'a');
		}

		// Check if character is a number
		if (c >= '0' && c <= '9') {
			return GLFW_KEY_0 + (c - '0');
		}

		// If the character is not an English letter or number, return -1 or some invalid code
		return -1;
	}
}

void Input::_newFrame() {
	scrollDelta = ScrollAccumulator;
	ScrollAccumulator = 0;
	
	currentFrameIndex = !currentFrameIndex;
	lastFrameIndex = !lastFrameIndex;

	std::copy(liveKeyStates, liveKeyStates + keyCount - 1, keyStates[currentFrameIndex]);
	std::fill(liveKeyStates, liveKeyStates + keyCount - 1, GLFW_RELEASE);

	std::copy(liveMouseBtnStates, liveMouseBtnStates + mouseBtnCount - 1, mouseBtnStates[currentFrameIndex]);
	std::fill(liveMouseBtnStates, liveMouseBtnStates + mouseBtnCount - 1, GLFW_RELEASE);
}

void Input::_onScroll(double xoffset, double yoffset) {
	ScrollAccumulator += yoffset;
}

void Input::_onKeyboard(int key, int scancode, int action, int mods) {
	liveKeyStates[key] |= (action == GLFW_PRESS);
}
void Input::_onMouseButton(int button, int action, int mods) {
	liveMouseBtnStates[button] |= (action == GLFW_PRESS);
}


bool Input::getKey(char key) {
	int gkey = AsciiToGLFWKey(key);
	assert(gkey != -1);
	return glfwGetKey(window, gkey);

}
bool Input::getKeyDown(char key) {
	int keyi = AsciiToGLFWKey(key);
	return keyStates[lastFrameIndex][keyi] == GLFW_RELEASE && keyStates[currentFrameIndex][keyi] == GLFW_PRESS;
}
//bool Input::getKeyUp(char key);

bool Input::getKey(KeyCode key) {
	return glfwGetKey(window, keyInputMap[key]);
}
bool Input::getKeyDown(KeyCode key) {
	auto keyi = keyInputMap[key];
	return keyStates[lastFrameIndex][keyi] == GLFW_RELEASE && keyStates[currentFrameIndex][keyi] == GLFW_PRESS;
}
//bool Input::getKeyUp(KeyCode key);

glm::vec2 Input::getMousePos() {
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	return glm::vec2((float)x, (float)y);
}
bool Input::getMouseBtn(MouseBtn button) {
	return glfwGetMouseButton(window, mouseInputMap[button]);
}

bool Input::getMouseBtnDown(MouseBtn button) {
	return mouseBtnStates[lastFrameIndex][mouseInputMap[button]] == GLFW_RELEASE && 
		mouseBtnStates[currentFrameIndex][mouseInputMap[button]] == GLFW_PRESS;
}

bool Input::getMouseBtnUp(MouseBtn button) {
	return mouseBtnStates[lastFrameIndex][mouseInputMap[button]] == GLFW_PRESS &&
		mouseBtnStates[currentFrameIndex][mouseInputMap[button]] == GLFW_RELEASE;
}
