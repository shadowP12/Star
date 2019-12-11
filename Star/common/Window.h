#pragma once
#include <iostream>
#include "../glad/glad.h"
#include "../cl/CLCore.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <glfw/include/GLFW/glfw3.h>
#include <glfw/include/GLFW/glfw3native.h>
#include <functional>
class Window
{
public:
	Window(int width, int height);
	~Window();
	void run(std::function<void()> func);
	void initCLCore(rc::CLCore* core);
private:
	GLFWwindow* mWin;
	int mWidth;
	int mHeight;
};
