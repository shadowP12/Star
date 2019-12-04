#include <iostream>
#include "glad/glad.h"
#include <glfw/include/GLFW/glfw3.h>
#include "cl/RendererCL.h"
#include "Input/Input.h"
#include "Scene.h"
#include "BVH.h"
const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;

//窗口回调函数
void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Star", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "failed to create glfw window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);

	//初始化gl函数指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "failed to initialize glad" << std::endl;
		return -1;
	}
	std::shared_ptr<Scene> scene = std::shared_ptr<Scene>(new Scene());
	scene->load("E:/dev/star/Res/monkey.gltf");
	scene->genPrimitives();
	BVH* bvh = new BVH(scene->getPrimitives());
	rc::RendererCL renderer(SCR_WIDTH, SCR_HEIGHT, window, bvh);

	Input::startUp();
	while (!glfwWindowShouldClose(window))
	{
		renderer.run();
		Input::instance().update();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	Input::shutDown();
	delete bvh;
	return 0;
}

/*
#include <iostream>
#include "glad/glad.h"
#include <glfw/include/GLFW/glfw3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include "Scene.h"
#include "BVH.h"
#include "Input/Input.h"
#include "DebugDraw.h"
#include "ShaderProgram.h"
#include "Camera.h"
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

glm::vec3 kLightDir = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
glm::vec3 kLightColor = glm::vec3(0.9f, 0.9f, 0.9f);

//窗口回调函数
void cursorPosCallback(GLFWwindow * window, double xPos, double yPos);
void keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
void mouseScrollCallback(GLFWwindow * window, double xOffset, double yOffset);

glm::vec3 Trace(Ray& r, std::shared_ptr<BVH> bvh)
{
	Intersection in;
	bool ret = bvh->intersect(r,in);
	if (ret)
	{
		glm::vec3 target = in.mPos + in.mNormal;
		glm::vec3 albedo = glm::vec3(0.9, 0.9, 0.9);
		glm::vec3 ambient = 0.2f * kLightColor;
		glm::vec3 n = glm::normalize(in.mNormal);
		glm::vec3 diffuse = kLightColor * (fmax(0.0f, glm::dot(-kLightDir, n)));
		return (ambient + diffuse) * albedo;
	}

	return glm::vec3(0,0,0);
}
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

	//初始化gl函数指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	//
	Input::startUp();
	DebugDraw::startUp();
	std::shared_ptr<Camera> camera = std::shared_ptr<Camera>(new Camera(glm::vec3(0,0,10), SCR_WIDTH, SCR_HEIGHT,45,0.1,100));
	std::shared_ptr<Scene> scene = std::shared_ptr<Scene>(new Scene());
	scene->load("E:/dev/star/Res/monkey.gltf");
	scene->genPrimitives();
	std::shared_ptr<BVH> bvh = std::shared_ptr<BVH>(new BVH(scene->getPrimitives()));
	std::shared_ptr<ShaderProgram> sp1 = std::shared_ptr<ShaderProgram>(new ShaderProgram("E:/dev/star/Res/Shader/default.vs","E:/dev/star/Res/Shader/default.fs"));
	std::shared_ptr<ShaderProgram> sp2 = std::shared_ptr<ShaderProgram>(new ShaderProgram("E:/dev/star/Res/Shader/debug.vs", "E:/dev/star/Res/Shader/debug.fs"));

	while (!glfwWindowShouldClose(window))
	{
		camera->tick();
		
		if (Input::instance().getMouseButtonDown(MouseButton::MouseLeft))
		{
			Ray ray;
			camera->GenerateRay(Input::instance().getMousePosition().x, Input::instance().getMousePosition().y, ray);
			Trace(ray,bvh);
			glm::vec3 t = ray.pointAt(1000);
			DebugDraw::instance().addLine(&ray.mOrig[0],&t[0],&glm::vec3(1,0,0)[0]);
		}
		Input::instance().update();
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		sp1->use();
		sp1->setMat4("view",camera->getViewMatrix());
		sp1->setMat4("projection", camera->getProjMatrix());
		scene->draw(sp1);

		sp2->use();
		sp2->setMat4("view", camera->getViewMatrix());
		sp2->setMat4("projection", camera->getProjMatrix());
		DebugDraw::instance().draw();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	Input::shutDown();
	DebugDraw::shutDown();
	uint8_t* image = new uint8_t[SCR_WIDTH * SCR_HEIGHT * 4];
	uint8_t* data = image;

	for (int y = 0; y < SCR_HEIGHT; ++y)
	{
		for (int x = 0; x < SCR_WIDTH; ++x)
		{
			glm::vec3 col(0, 0, 0);

			Ray ray;
			camera->GenerateRay(x, y, ray);
			col = Trace(ray,bvh);

			col.x = sqrtf(col.x);
			col.y = sqrtf(col.y);
			col.z = sqrtf(col.z);

			data[0] = uint8_t(saturate(col.x) * 255.0f);
			data[1] = uint8_t(saturate(col.y) * 255.0f);
			data[2] = uint8_t(saturate(col.z) * 255.0f);
			data[3] = 255;
			data += 4;
		}
	}
	stbi_flip_vertically_on_write(0);
	stbi_write_png("output.png", SCR_WIDTH, SCR_HEIGHT, 4, image, SCR_WIDTH * 4);
	delete image;
	return 0;
}

*/

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
