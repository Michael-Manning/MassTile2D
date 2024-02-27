#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

//for input rollover and events
struct inputDescription {
	int btn, action, read;
	bool operator==(const inputDescription& b) {
		return (action == b.action && btn == b.btn);
	}
};


constexpr int keyCount = 348;
constexpr int mouseBtnCount = 8;

enum class KeyCode {
	LeftControl,
	RightControl,
	LeftArrow,
	RightArrow,
	UpArrow,
	DownArrow,
	Spacebar
};

enum class MouseBtn {
	Left,
	Right
};

// TODO add mutex to _newFrame() to prevent dropped events when GLFW executes a callback

class Input {
public:

	Input(GLFWwindow* window) {
		this->window = window;

		std::fill(liveMouseBtnStates, liveMouseBtnStates + mouseBtnCount - 1, GLFW_FALSE);
		std::fill(liveKeyStates, liveKeyStates + keyCount - 1, GLFW_FALSE);
	}

	float GetScrollDelta() {
		return scrollDelta;
	}


	void _newFrame();

	//input manager stuff
	void _onKeyboard(int key, int scancode, int action, int mods);
	void _onMouseButton(int button, int action, int mods);
	//void _onCursor();
	void _onScroll(double xoffset, double yoffset);


	bool getKey(char key);
	bool getKeyDown(char key);
	//bool getKeyUp(char key);

	bool getKey(KeyCode key);
	bool getKeyDown(KeyCode key);
	//bool getKeyUp(KeyCode key);

	glm::vec2 getMousePos(); // rename as live version. add per frame cached version for behaviour consistency


	bool getMouseBtn(MouseBtn button);
	bool getMouseBtnDown(MouseBtn button);
	bool getMouseBtnUp(MouseBtn button);
		
private:
	GLFWwindow* window;

	int currentFrameIndex = 1;
	int lastFrameIndex = 0;

	int ar1[keyCount];
	int ar2[keyCount];

	int liveKeyStates[keyCount];
	int keyStates[2][keyCount];
	// int LeftMouseState, RightMouseState;

	int liveMouseBtnStates[mouseBtnCount];
	int mouseBtnStates[2][mouseBtnCount];

	double ScrollAccumulator = 0.0;
	float scrollDelta = 0.0f;
};