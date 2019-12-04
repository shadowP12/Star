#include "RendererCL.h"
#include "../tools/Tools.h"
#include "../ShaderProgram.h"
#include "../Input/Input.h"
#include <vector>

float QuadVertices[] = 
{
	 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	-1.0f,  1.0f, 0.0f, 0.0f, 1.0f 
};

unsigned int Indices[] = 
{
	0, 1, 3,
	1, 2, 3
};

unsigned int framenumber = 0;

RAY_CL_NAMESPACE_BEGIN

RendererCL::RendererCL(int width, int height, GLFWwindow* win, BVH* bvh)
{
	resize(width, height);
	// get platform
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.empty()) throw std::runtime_error("cannot get opencl platform!");

	int platformIndex = 0;
	for (size_t i = 0; i < platforms.size(); i++) 
	{
		std::string s = platforms[i].getInfo<CL_PLATFORM_VENDOR>();
		if (s.find("NVIDIA") != std::string::npos || s.find("AMD") != std::string::npos) 
		{
			platformIndex = (int)i;
			break;
		}
	}
	mPlatform = platforms[platformIndex];

	// get device
	std::vector<cl::Device> devices;
	mPlatform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	if (devices.empty()) throw std::runtime_error("cannot get opencl device!");
	if (!checkExtnAvailability(devices[0], "cl_khr_gl_sharing")) throw std::runtime_error("cannot get opencl device!");

	mDevice = devices[0];

	cl_int error = CL_SUCCESS;
	cl_context_properties cps[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetWGLContext(win),
			CL_WGL_HDC_KHR, (cl_context_properties)GetDC(glfwGetWin32Window(win)),
			CL_CONTEXT_PLATFORM, (cl_context_properties)mPlatform(),
			0
	};
	mContext = cl::Context(devices, cps, nullptr, nullptr, &error);
	if (error != CL_SUCCESS) throw std::runtime_error("cannot create opencl context!");

	mQueue = cl::CommandQueue(mContext, mDevice, CL_QUEUE_PROFILING_ENABLE, &error);

	if (error != CL_SUCCESS) throw std::runtime_error("cannot create opencl queue!");

	// create program
	std::string path = "E:/dev/star/Kernels/base.cl";
	std::string source;
	readFileData(path, source);
	const char* cSource = source.c_str();

	mProgram = cl::Program(mContext, cSource);
 
	cl_int result = mProgram.build({ mDevice }, "-I E:/dev/star/Kernels");

	if (result == CL_BUILD_PROGRAM_FAILURE)
	{
		std::string buildLog = mProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(mDevice);
		printf("%s\n", buildLog.c_str());
	}
	
	// gl
	mDisplayProgram  = std::shared_ptr<ShaderProgram>(new ShaderProgram("E:/dev/star/Res/Shader/texture.vs", "E:/dev/star/Res/Shader/texture.fs"));

	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mEBO);

	glBindVertexArray(mVAO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), QuadVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture
	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

	glGenBuffers(1, &mPBO);
	glBindBuffer(GL_ARRAY_BUFFER, mPBO);
	glBufferData(GL_ARRAY_BUFFER, mWidth * mHeight * sizeof(cl_uchar4), 0, GL_DYNAMIC_DRAW);

	cl_int errCode;

	mPixelBuffer = cl::BufferGL(mContext, CL_MEM_WRITE_ONLY, mPBO, NULL);

	mImage = cl::ImageGL(mContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, mTexture, &errCode);
	if (errCode != CL_SUCCESS) 
	{
		std::cout << "Failed to create OpenGL texture refrence: " << errCode << std::endl;
	}

	mMemorys.push_back(mImage);

	mSpheres = new GPUVector<Sphere>(mContext, mQueue, CL_MEM_READ_ONLY);
	mBVHNodes = new GPUVector<BVHNode>(mContext, mQueue, CL_MEM_READ_ONLY);
	initBVH(bvh);
	initScene();
	initKernel();

	mCPUCamera.position = glm::vec3(0,0,0);
	mCPUCamera.front = glm::vec3(0, 0, -1);
	mCPUCamera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	mCPUCamera.yaw = -90.0f;
	mCPUCamera.pitch = 0.0f;
}

RendererCL::~RendererCL() 
{
	glDeleteVertexArrays(1, &mVAO);
	glDeleteBuffers(1, &mVBO);
	glDeleteBuffers(1, &mEBO);
	glDeleteBuffers(1, &mPBO);
	delete mSpheres;
	delete mBVHNodes;
}

void RendererCL::resize(int width, int height)
{
	mWidth = width;
	mHeight = height;
}

void RendererCL::initKernel()
{
	unsigned int rendermode = 1;
	mKernel = cl::Kernel(mProgram, "render_kernel");

	mKernel.setArg(0, mSpheres->getBuffer());
	mKernel.setArg(1, mWidth);
	mKernel.setArg(2, mHeight);
	mKernel.setArg(3, 9);
	mKernel.setArg(4, mImage);
	mKernel.setArg(5, 0);
}

#define float3(x, y, z) {{x, y, z}}

void RendererCL::initScene()
{
	// left wall
	mCpuSpheres[0].radius = 200.0f;
	mCpuSpheres[0].position = float3(-200.6f, 0.0f, 0.0f);
	mCpuSpheres[0].color = float3(0.75f, 0.25f, 0.25f);
	mCpuSpheres[0].emission = float3(0.0f, 0.0f, 0.0f);

	// right wall
	mCpuSpheres[1].radius = 200.0f;
	mCpuSpheres[1].position = float3(200.6f, 0.0f, 0.0f);
	mCpuSpheres[1].color = float3(0.25f, 0.25f, 0.75f);
	mCpuSpheres[1].emission = float3(0.0f, 0.0f, 0.0f);

	// floor
	mCpuSpheres[2].radius = 200.0f;
	mCpuSpheres[2].position = float3(0.0f, -200.4f, 0.0f);
	mCpuSpheres[2].color = float3(0.9f, 0.8f, 0.7f);
	mCpuSpheres[2].emission = float3(0.0f, 0.0f, 0.0f);

	// ceiling
	mCpuSpheres[3].radius = 200.0f;
	mCpuSpheres[3].position = float3(0.0f, 200.4f, 0.0f);
	mCpuSpheres[3].color = float3(0.9f, 0.8f, 0.7f);
	mCpuSpheres[3].emission = float3(0.0f, 0.0f, 0.0f);

	// back wall
	mCpuSpheres[4].radius = 200.0f;
	mCpuSpheres[4].position = float3(0.0f, 0.0f, -200.4f);
	mCpuSpheres[4].color = float3(0.9f, 0.8f, 0.7f);
	mCpuSpheres[4].emission = float3(0.0f, 0.0f, 0.0f);

	// front wall 
	mCpuSpheres[5].radius = 200.0f;
	mCpuSpheres[5].position = float3(0.0f, 0.0f, 202.0f);
	mCpuSpheres[5].color = float3(0.9f, 0.8f, 0.7f);
	mCpuSpheres[5].emission = float3(0.0f, 0.0f, 0.0f);

	// left sphere
	mCpuSpheres[6].radius = 0.16f;
	mCpuSpheres[6].position = float3(-0.25f, -0.24f, -0.1f);
	mCpuSpheres[6].color = float3(0.9f, 0.8f, 0.7f);
	mCpuSpheres[6].emission = float3(0.0f, 0.0f, 0.0f);

	// right sphere
	mCpuSpheres[7].radius = 0.16f;
	mCpuSpheres[7].position = float3(0.25f, -0.24f, 0.1f);
	mCpuSpheres[7].color = float3(0.9f, 0.8f, 0.7f);
	mCpuSpheres[7].emission = float3(0.0f, 0.0f, 0.0f);

	// lightsource
	mCpuSpheres[8].radius = 1.0f;
	mCpuSpheres[8].position = float3(0.0f, 1.36f, 0.0f);
	mCpuSpheres[8].color = float3(0.0f, 0.0f, 0.0f);
	mCpuSpheres[8].emission = float3(9.0f, 8.0f, 6.0f);

	mSpheres->pushBack(mCpuSpheres[0]);
	mSpheres->pushBack(mCpuSpheres[1]);
	mSpheres->pushBack(mCpuSpheres[2]);
	mSpheres->pushBack(mCpuSpheres[3]);
	mSpheres->pushBack(mCpuSpheres[4]);
	mSpheres->pushBack(mCpuSpheres[5]);
	mSpheres->pushBack(mCpuSpheres[6]);
	mSpheres->pushBack(mCpuSpheres[7]);
	mSpheres->pushBack(mCpuSpheres[8]);

	mSpheresBuffer = cl::Buffer(mContext, CL_MEM_WRITE_ONLY, 9 * sizeof(Sphere));
	mQueue.enqueueWriteBuffer(mSpheresBuffer, CL_TRUE, 0, 9 * sizeof(Sphere), mCpuSpheres);

	mCameraBuffer = cl::Buffer(mContext, CL_MEM_WRITE_ONLY, sizeof(Camera));
}
inline unsigned divup(unsigned a, unsigned b)
{
    return (a+b-1)/b;
}

unsigned int WangHash(unsigned int a) 
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

void RendererCL::run()
{
	updateCamera();
	mGPUCamera.orig = { {mCPUCamera.position.x, mCPUCamera.position.y, mCPUCamera.position.z} };
	mGPUCamera.front = { {mCPUCamera.front.x, mCPUCamera.front.y, mCPUCamera.front.z} };
	mGPUCamera.up = { {mCPUCamera.up.x, mCPUCamera.up.y, mCPUCamera.up.z} };
	mGPUCamera.params = { {45.0f, 45.0f, 0.001f, 0.001f} };
	mQueue.enqueueWriteBuffer(mCameraBuffer, CL_TRUE, 0, sizeof(Camera), &mGPUCamera);
	//
	
	mKernel.setArg(5, WangHash(framenumber));
	mKernel.setArg(6, mCameraBuffer);

	std::size_t global_work_size = mWidth * mHeight;
	std::size_t local_work_size = mKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(mDevice);
	cl_int error;
	if (global_work_size % local_work_size != 0)
		global_work_size = (global_work_size / local_work_size + 1) * local_work_size;

	glFinish();
	mQueue.enqueueAcquireGLObjects(&mMemorys);
	mQueue.finish();
	cl::NDRange local(16, 16);
	cl::NDRange global(local[0] * divup(mWidth, local[0]),
		local[1] * divup(mHeight, local[1]));
	// launch the kernel
	mQueue.enqueueNDRangeKernel(mKernel, cl::NullRange, global, local);
	mQueue.finish();

	mQueue.enqueueReleaseGLObjects(&mMemorys);
	mQueue.finish();

	glClearColor(0.3f, 0.3f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	mDisplayProgram->use();
	glActiveTexture(GL_TEXTURE0);
	mDisplayProgram->setInt("sTexture", 0);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

bool RendererCL::checkExtnAvailability(cl::Device device, std::string name)
{
	bool ret_val = true;
	std::string exts = device.getInfo<CL_DEVICE_EXTENSIONS>();
	std::stringstream ss(exts);
	std::string item;
	int found = -1;
	while (std::getline(ss, item, ' ')) 
	{
		if (item == name) {
			return true;
			break;
		}
	}
	return false;
}

void RendererCL::updateCamera()
{
	glm::vec2 offset = Input::instance().getMousePosition() - mCPUCamera.lastMousePosition;
	mCPUCamera.lastMousePosition = Input::instance().getMousePosition();
	if (Input::instance().getMouseButton(MouseButton::MouseRight))
	{
		mCPUCamera.yaw += -offset.x * 0.1f;
		mCPUCamera.pitch += -offset.y * 0.1f;
		glm::vec3 front;
		front.x = cos(glm::radians(mCPUCamera.yaw)) * cos(glm::radians(mCPUCamera.pitch));
		front.y = sin(glm::radians(mCPUCamera.pitch));
		front.z = sin(glm::radians(mCPUCamera.yaw)) * cos(glm::radians(mCPUCamera.pitch));
		mCPUCamera.front = glm::normalize(front);
		mCPUCamera.right = glm::normalize(glm::cross(mCPUCamera.front, glm::vec3(0, 1, 0)));
		mCPUCamera.up = glm::normalize(glm::cross(mCPUCamera.right, mCPUCamera.front));
	}
	if (Input::instance().getMouseScrollWheel() != 0)
	{
		float sw = Input::instance().getMouseScrollWheel();
		mCPUCamera.position += mCPUCamera.front * sw * 0.1f;
	}
}

void RendererCL::initBVH(BVH* bvh)
{
	LinearBVHNode* nodes = bvh->getNodes();
	int nodeCount = bvh->getNodeCount();
	for (int i = 0; i < nodeCount; i++)
	{
		BVHNode node;
		node.axis = nodes[i].axis;
		node.numPrimitive = nodes[i].numPrimitive;
		node.primitiveOffset = nodes[i].primitiveOffset;
		node.secondChildOffset = nodes[i].secondChildOffset;
		node.bboxMin = { {nodes[i].bound.mMin.x, nodes[i].bound.mMin.y, nodes[i].bound.mMin.z} };
		node.bboxMax = { {nodes[i].bound.mMax.x, nodes[i].bound.mMax.y, nodes[i].bound.mMax.z} };
		mBVHNodes->pushBack(node);
	}
}

RAY_CL_NAMESPACE_END