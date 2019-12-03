#pragma once
#include "CoreCL.h"
#include "DataCL.h"
#include "../glad/glad.h"
#include <memory>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <glfw/include/GLFW/glfw3.h>
#include <glfw/include/GLFW/glfw3native.h>
#include "../GMath.h"
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


RAY_CL_NAMESPACE_BEGIN

struct Sphere
{
	cl_float radius;
	cl_float dummy1;
	cl_float dummy2;
	cl_float dummy3;
	cl_float3 position;
	cl_float3 color;
	cl_float3 emission;
};

class RendererCL
{
public:
	RendererCL(int width, int height, GLFWwindow* win);
	~RendererCL();
	void resize(int width, int height);
	void run();
private:
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
