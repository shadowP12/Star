#pragma once
#include "CLCore.h"
#include "CLDatas.h"
#include "CLTools.h"
#include "../glad/glad.h"
#include <memory>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <glfw/include/GLFW/glfw3.h>
#include <glfw/include/GLFW/glfw3native.h>
#include "../GMath.h"
#include "../BVH.h"
class ShaderProgram;

struct CPUCamera
{
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec2 lastMousePosition;
	float yaw;
	float pitch;
};


RC_NAMESPACE_BEGIN

class RendererCL
{
public:
	RendererCL(int width, int height, GLFWwindow* win, BVH* bvh);
	~RendererCL();
	void resize(int width, int height);
	void run();
private:
	void initBVH(BVH* bvh);
	void initKernel();
	void initScene();
	bool checkExtnAvailability(cl::Device device, std::string name);
	void updateCamera();
private:
	cl::Platform mPlatform;
	cl::Device mDevice;
	cl::Context mContext;
	cl::Program mProgram;
	cl::CommandQueue mQueue;
	cl::Kernel mKernel;
	cl::BufferGL mPixelBuffer;
	cl::ImageGL mImage;
	std::vector<cl::Memory> mMemorys;
	Sphere mCpuSpheres[9];
	GPUVector<Sphere>* mSpheres;
	GPUVector<BVHNode>* mBVHNodes;
	cl::Buffer mSpheresBuffer;
	cl::Buffer mCameraBuffer;
	GLuint mTexture;
	GLuint mVAO, mVBO, mEBO, mPBO;
	std::shared_ptr<ShaderProgram> mDisplayProgram;
	int mWidth;
	int mHeight;
	Camera mGPUCamera;
	CPUCamera mCPUCamera;
};

RAY_CL_NAMESPACE_END
