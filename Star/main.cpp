#include <iostream>
#define GLEW_STATIC
#include <glew/include/GL/glew.h>
#include <glfw/include/GLFW/glfw3.h>
#include "Scene.h"
#include "BVH.h"
#include "Input/Input.h"
#include "DebugDraw.h"
#include "ShaderProgram.h"
#include "Camera.h"
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

//窗口回调函数
void cursorPosCallback(GLFWwindow * window, double xPos, double yPos);
void keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
void mouseScrollCallback(GLFWwindow * window, double xOffset, double yOffset);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Star", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK) 
	{
		std::cout << "Failed to initialize GLEW : " << glewGetErrorString(err) <<std::endl;
		getchar();
		return  -1;
	}
	glEnable(GL_DEPTH_TEST);

	//
	Input::startUp();
	DebugDraw::startUp();
	std::shared_ptr<Camera> camera = std::shared_ptr<Camera>(new Camera(glm::vec3(0,0,10)));
	std::shared_ptr<Scene> scene = std::shared_ptr<Scene>(new Scene());
	scene->load("F:/Dev/Star/Res/monkey.gltf");
	scene->genPrimitives();
	std::shared_ptr<BVH> bvh = std::shared_ptr<BVH>(new BVH(scene->getPrimitives()));
	std::shared_ptr<ShaderProgram> sp1 = std::shared_ptr<ShaderProgram>(new ShaderProgram("F:/Dev/Star/Res/Shader/default.vs","F:/Dev/Star/Res/Shader/default.fs"));
	std::shared_ptr<ShaderProgram> sp2 = std::shared_ptr<ShaderProgram>(new ShaderProgram("F:/Dev/Star/Res/Shader/debug.vs", "F:/Dev/Star/Res/Shader/debug.fs"));

	while (!glfwWindowShouldClose(window))
	{
		camera->tick();
		Input::instance().update();

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		sp1->use();
		sp1->setMat4("view",camera->getViewMatrix());
		sp1->setMat4("projection", projection);
		scene->draw(sp1);

		sp2->use();
		sp2->setMat4("view", camera->getViewMatrix());
		sp2->setMat4("projection", projection);
		DebugDraw::instance().draw();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	Input::shutDown();
	DebugDraw::shutDown();
	return 0;
}

void cursorPosCallback(GLFWwindow * window, double xPos, double yPos)
{
	Input::instance().setMousePosition(glm::vec2(xPos, yPos));
}

void keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods)
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

void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
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

void mouseScrollCallback(GLFWwindow * window, double xOffset, double yOffset)
{
	Input::instance().setMouseScrollWheel((float)yOffset);
}