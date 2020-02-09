#include "RendererCL.h"
#include "../tools/Tools.h"
#include "../ShaderProgram.h"
#include "../InputSystem/Input.h"
#include <memory>
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

RC_NAMESPACE_BEGIN

RendererCL::RendererCL(int width, int height)
{
    mFrameCount = 0;
	resize(width, height);
}

RendererCL::~RendererCL() 
{
	glDeleteVertexArrays(1, &mVAO);
	glDeleteBuffers(1, &mVBO);
	glDeleteBuffers(1, &mEBO);
	delete mBVHNodes;
	delete mTriangles;
	delete mMaterials;
}

void RendererCL::resize(int width, int height)
{
	mWidth = width;
	mHeight = height;
}

void RendererCL::initCL(CLCore* core)
{
	mCore = core;
	// create program
	std::string path = "Kernels/base.cl";
	std::string source;
	readFileData(path, source);
	mProgram = cl::Program(mCore->context, source.c_str());
	cl_int result = mProgram.build({ mCore->device }, "-I Kernels");

    std::string buildLog = mProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(mCore->device);

	if (result == CL_BUILD_PROGRAM_FAILURE)
	{
        printf("%s\n", buildLog.c_str());
	}

	// 初始化gl资源
	mDisplayProgram = std::make_shared<ShaderProgram>("Res/Shader/texture.vs", "Res/Shader/texture.fs");

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

	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

	cl_int errCode;
	mImage = cl::ImageGL(mCore->context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, mTexture, &errCode);
	if (errCode != CL_SUCCESS)
	{
		printf("failed to create opengl texture refrence:%d", errCode);
	}
	
	mMemorys.push_back(mImage);

	// 初始化Kernel
	mKernel = cl::Kernel(mProgram, "renderKernel");

	// 初始化容器
	mBVHNodes = new GPUVector<CLBVHNode>(mCore);
	mTriangles = new GPUVector<CLTriangle>(mCore);
	mMaterials = new GPUVector<CLMaterial>(mCore);
}

void RendererCL::initScene(BVH* bvh, std::vector<std::shared_ptr<Material>>& mats)
{
	// 初始化相机
	mCPUCamera.position = glm::vec3(0, 0, 0);
	mCPUCamera.front = glm::vec3(0, 0, -1);
	mCPUCamera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	mCPUCamera.yaw = -90.0f;
	mCPUCamera.pitch = 0.0f;
	mCameraBuffer = cl::Buffer(mCore->context,CL_MEM_WRITE_ONLY, sizeof(CLCamera));

	// 初始化bvh节点
	LinearBVHNode* nodes = bvh->getNodes();
	int nodeCount = bvh->getNodeCount();
	int max = 0;
	for (int i = 0; i < nodeCount; i++)
	{
		if (max < nodes[i].primitiveOffset + nodes[i].numPrimitive)
			max = nodes[i].primitiveOffset + nodes[i].numPrimitive;
		CLBVHNode node;
		node.axis = nodes[i].axis;
		node.numPrimitive = nodes[i].numPrimitive;
		node.primitiveOffset = nodes[i].primitiveOffset;
		node.secondChildOffset = nodes[i].secondChildOffset;
		node.bboxMin = { {nodes[i].bound.mMin.x, nodes[i].bound.mMin.y, nodes[i].bound.mMin.z} };
		node.bboxMax = { {nodes[i].bound.mMax.x, nodes[i].bound.mMax.y, nodes[i].bound.mMax.z} };
		mBVHNodes->pushBack(node);
	}

	// 初始化三角网格数据
	std::vector<std::shared_ptr<Primitive>> prims = bvh->getPrims();
	for (int i = 0; i < prims.size(); i++)
	{
		glm::vec3 p0, p1, p2;
        glm::vec3 n0, n1, n2;
        glm::vec2 t0, t1, t2;
		prims[i]->getTri()->getPositionData(p0, p1, p2);
        prims[i]->getTri()->getNormalData(n0, n1, n2);
        prims[i]->getTri()->getUVData(t0, t1, t2);

		CLVertex v0, v1, v2;
		v0.position = { {p0.x, p0.y, p0.z} };
		v0.normal = { {n0.x, n0.y, n0.z} };
		v0.texcoord = { {t0.x, t0.y, 0.0} };

        v1.position = { {p1.x, p1.y, p1.z} };
        v1.normal = { {n1.x, n1.y, n1.z} };
        v1.texcoord = { {t1.x, t1.y, 0.0} };

        v2.position = { {p2.x, p2.y, p2.z} };
        v2.normal = { {n2.x, n2.y, n2.z} };
        v2.texcoord = { {t2.x, t2.y, 0.0} };

		CLTriangle tri;
		tri.v0 = v0;
		tri.v1 = v1;
		tri.v2 = v2;
		tri.mat = prims[i]->getTri()->getMaterialID();
		mTriangles->pushBack(tri);
	}
    // seting gpu material data
    for (int i = 0; i < mats.size(); ++i)
    {
        CLMaterial mat;
        mat.baseColor = { {mats[i]->baseColor[0], mats[i]->baseColor[1], mats[i]->baseColor[2]} };
        mat.emissive = { {mats[i]->emissive[0], mats[i]->emissive[1], mats[i]->emissive[2]} };
        mMaterials->pushBack(mat);
    }

	// 设置GPU数据
	updateCamera();
	mGPUCamera.orig = { {mCPUCamera.position.x, mCPUCamera.position.y, mCPUCamera.position.z} };
	mGPUCamera.front = { {mCPUCamera.front.x, mCPUCamera.front.y, mCPUCamera.front.z} };
	mGPUCamera.up = { {mCPUCamera.up.x, mCPUCamera.up.y, mCPUCamera.up.z} };
	mGPUCamera.params = { {45.0f, 45.0f, 0.001f, 0.001f} };
	mCore->queue.enqueueWriteBuffer(mCameraBuffer, CL_TRUE, 0, sizeof(CLCamera), &mGPUCamera);

	// 更新GPU数据
	mKernel.setArg(0, mWidth);
	mKernel.setArg(1, mHeight);
	mKernel.setArg(2, mCameraBuffer);
	mKernel.setArg(3, mBVHNodes->getBuffer());
	mKernel.setArg(4, mTriangles->getBuffer());
    mKernel.setArg(5, mMaterials->getBuffer());
	mKernel.setArg(7, mImage);
}

#define float3(x, y, z) {{x, y, z}}

inline unsigned divup(unsigned a, unsigned b)
{
    return (a+b-1)/b;
}

void RendererCL::run()
{
	// 更新相机数据
	updateCamera();
	mGPUCamera.orig = { {mCPUCamera.position.x, mCPUCamera.position.y, mCPUCamera.position.z} };
	mGPUCamera.front = { {mCPUCamera.front.x, mCPUCamera.front.y, mCPUCamera.front.z} };
	mGPUCamera.up = { {mCPUCamera.up.x, mCPUCamera.up.y, mCPUCamera.up.z} };
	mGPUCamera.params = { {45.0f, 45.0f, 0.001f, 0.001f} };
	mCore->queue.enqueueWriteBuffer(mCameraBuffer, CL_TRUE, 0, sizeof(CLCamera), &mGPUCamera);
	
	// 更新GPU数据
	mKernel.setArg(2, mCameraBuffer);
    mKernel.setArg(6, mFrameCount);

	std::size_t globalWorkSize = mWidth * mHeight;
	std::size_t localWorkSize = mKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(mCore->device);
	cl_int error;
	if (globalWorkSize % localWorkSize != 0)
		globalWorkSize = (localWorkSize / localWorkSize + 1) * localWorkSize;
	glFinish();
	mCore->queue.enqueueAcquireGLObjects(&mMemorys);
	mCore->queue.finish();
	cl::NDRange local(16, 16);
	cl::NDRange global(local[0] * divup(mWidth, local[0]), local[1] * divup(mHeight, local[1]));

	// 执行Kernel
	mCore->queue.enqueueNDRangeKernel(mKernel, cl::NullRange, global, local);
	mCore->queue.finish();

	mCore->queue.enqueueReleaseGLObjects(&mMemorys);
	mCore->queue.finish();

	// 渲染到屏幕上
	glClearColor(0.3f, 0.3f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	mDisplayProgram->use();
	glActiveTexture(GL_TEXTURE0);
	mDisplayProgram->setInt("sTexture", 0);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    mFrameCount++;
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
RC_NAMESPACE_END