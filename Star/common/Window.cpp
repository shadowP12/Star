#include "Window.h"
#include "../input/Input.h"
//窗口回调函数
void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

Window::Window(int width, int height)
	:mWidth(width),mHeight(height)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	mWin = glfwCreateWindow(mWidth, mHeight, "Star", NULL, NULL);
	if (mWin == NULL)
	{
		printf("failed to create glfw window\n");
		glfwTerminate();
	}

	glfwMakeContextCurrent(mWin);
	glfwSetKeyCallback(mWin, keyCallback);
	glfwSetMouseButtonCallback(mWin, mouseButtonCallback);
	glfwSetCursorPosCallback(mWin, cursorPosCallback);
	glfwSetScrollCallback(mWin, mouseScrollCallback);

	//初始化gl函数指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("failed to initialize glad\n");
	}
}

Window::~Window()
{
	glfwDestroyWindow(mWin);
}

void Window::run(std::function<void()> func)
{
	while (!glfwWindowShouldClose(mWin))
	{
		func();
		Input::instance().update();
		glfwSwapBuffers(mWin);
		glfwPollEvents();
	}
	glfwTerminate();
}

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos)
{
	Input::instance().setMousePosition(glm::vec2(xPos, yPos));
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	int curKey = 0;
	if (key >= 48 && key <= 57)
	{
		curKey = key - 48;
	}
	else if (key >= 65 && key <= 90)
	{
		curKey = key - 55;
	}
	else
	{
		return;
	}

	switch (action)
	{
	case GLFW_PRESS:
		Input::instance().setKeyDown(curKey);
		Input::instance().setKey(curKey, true);
		break;
	case GLFW_RELEASE:
		Input::instance().setKeyUp(curKey);
		Input::instance().setKey(curKey, false);
		break;
	default:
		break;
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (action)
	{
	case GLFW_PRESS:
		Input::instance().setMouseButtonDown(button);
		Input::instance().setMouseButton(button, true);
		break;
	case GLFW_RELEASE:
		Input::instance().setMouseButtonUp(button);
		Input::instance().setMouseButton(button, false);
		break;
	default:
		break;
	}
}

void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	Input::instance().setMouseScrollWheel((float)yOffset);
}
