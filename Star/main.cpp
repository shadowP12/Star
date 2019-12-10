#include <iostream>
#include "glad/glad.h"
#include <glfw/include/GLFW/glfw3.h>
#include "cl/RendererCL.h"
#include "Scene.h"
#include "BVH.h"
const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;

int main()
{
	std::shared_ptr<Scene> scene = std::shared_ptr<Scene>(new Scene());
	scene->load("E:/dev/star/Res/monkey.gltf");
	scene->genPrimitives();
	BVH* bvh = new BVH(scene->getPrimitives());
	rc::RendererCL renderer(SCR_WIDTH, SCR_HEIGHT, window, bvh);

	Input::startUp();

	Input::shutDown();
	delete bvh;
	return 0;
}