#include <iostream>
#include <memory>
#include "GLAD/glad.h"
#include <GLFW/glfw3.h>
#include "CL/RendererCL.h"
#include "Scene.h"
#include "BVH.h"
#include "InputSystem/Input.h"
#include "Common/Window.h"
#include "Math/GMath.h"
const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;

rc::CLCore* gCLCore = nullptr;

void printMat(glm::mat4& mat)
{
    printf("%f  %f  %f  %f\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
    printf("%f  %f  %f  %f\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3]);
    printf("%f  %f  %f  %f\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3]);
    printf("%f  %f  %f  %f\n", mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
}

int main()
{
    // test
//    printf("cl_float %d\n", sizeof(cl_float));
//    printf("cl_float3 %d\n", sizeof(cl_float3));
//    printf("CLVertex %d\n", sizeof(rc::CLVertex));
//    printf("CLTriangle %d\n", sizeof(rc::CLTriangle));

	Input::startUp();
	gCLCore = new rc::CLCore();
	Window win(SCR_WIDTH, SCR_HEIGHT);
	win.initCLCore(gCLCore);
	std::shared_ptr<Scene> scene = std::make_shared<Scene>();
	scene->load("Res/monkey.gltf");
	scene->genPrimitives();
	BVH* bvh = new BVH(scene->getPrimitives());
	// renderer
	rc::RendererCL renderer(SCR_WIDTH, SCR_HEIGHT);
	renderer.initCL(gCLCore);
	renderer.initScene(bvh, scene->getMaterials());

	win.run([&renderer] {renderer.run(); });

	Input::shutDown();
	delete bvh;
	delete gCLCore;

	return 0;
}