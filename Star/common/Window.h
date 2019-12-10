#pragma once
#include <iostream>
#include "../glad/glad.h"
#include "../cl/CLCore.h"
#include <glfw/include/GLFW/glfw3.h>
#include <functional>
class Window
{
public:
	Window(int width, int height);
	~Window();
	void run(std::function<void()> func);
	
private:
	GLFWwindow* mWin;
	int mWidth;
	int mHeight;
};
