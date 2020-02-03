#include "Window.h"
#include "../InputSystem/Input.h"
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

void Window::initCLCore(rc::CLCore* core)
{
	// get platform
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.empty()) throw std::runtime_error("cannot get opencl platform!");

	int platformIndex = 0;
	for (int i = 0; i < platforms.size(); i++)
	{
		std::string info = platforms[i].getInfo<CL_PLATFORM_VENDOR>();
		if (info.find("NVIDIA") != std::string::npos || info.find("AMD") != std::string::npos)
		{
			platformIndex = (int)i;
			break;
		}
	}

	core->platform = platforms[platformIndex];

	// get device
	std::vector<cl::Device> devices;
	core->platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
	if (devices.empty()) throw std::runtime_error("cannot get opencl device!");
	core->device = devices[0];

	// get context
	cl_int error = CL_SUCCESS;
	cl_context_properties cps[] = 
	{
			CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetWGLContext(mWin),
			CL_WGL_HDC_KHR, (cl_context_properties)GetDC(glfwGetWin32Window(mWin)),
			CL_CONTEXT_PLATFORM, (cl_context_properties)core->platform(),
			0
	};

	core->context = cl::Context(devices, cps, nullptr, nullptr, &error);
	if (error != CL_SUCCESS) throw std::runtime_error("cannot create opencl context!");

	// gat command queue
	core->queue = cl::CommandQueue(core->context, core->device, CL_QUEUE_PROFILING_ENABLE, &error);
	if (error != CL_SUCCESS) throw std::runtime_error("cannot create opencl queue!");
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
