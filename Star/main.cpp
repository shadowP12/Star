#include <iostream>
#include "GLAD/glad.h"
#include <GLFW/glfw3.h>
#include "CL/RendererCL.h"
#include "Scene.h"
#include "BVH.h"
#include "InputSystem/Input.h"
#include "Common/Window.h"
const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;

rc::CLCore* gCLCore = nullptr;

int main()
{
	Input::startUp();
	gCLCore = new rc::CLCore();
	Window win(SCR_WIDTH, SCR_HEIGHT);
	win.initCLCore(gCLCore);
	std::shared_ptr<Scene> scene = std::shared_ptr<Scene>(new Scene());
	scene->load("Res/monkey.gltf");
	scene->genPrimitives();
	BVH* bvh = new BVH(scene->getPrimitives());
	// renderer
	rc::RendererCL renderer(SCR_WIDTH, SCR_HEIGHT);
	renderer.initCL(gCLCore);
	renderer.initScene(bvh);

	win.run([&renderer] {renderer.run(); });

	Input::shutDown();
	delete bvh;
	delete gCLCore;
	return 0;
}