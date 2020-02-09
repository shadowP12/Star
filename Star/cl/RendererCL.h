#pragma once
#include "CLCore.h"
#include "CLDatas.h"
#include "CLTools.h"
#include "../glad/glad.h"
#include "../math/GMath.h"
#include "../BVH.h"
#include <memory>
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
	RendererCL(int width, int height);
	~RendererCL();
	void resize(int width, int height);
	void run();
	void initCL(CLCore* core);
	void initScene(BVH* bvh, std::vector<std::shared_ptr<Material>>& mats);
private:
	void updateCamera();
private:
	CLCore* mCore;
	cl::Program mProgram;
	cl::Kernel mKernel;
	cl::ImageGL mImage;
	std::vector<cl::Memory> mMemorys;
	GPUVector<CLBVHNode>* mBVHNodes;
	GPUVector<CLTriangle>* mTriangles;
	GPUVector<CLMaterial>* mMaterials;
	cl::Buffer mCameraBuffer;
	GLuint mTexture;
	GLuint mVAO, mVBO, mEBO;
	std::shared_ptr<ShaderProgram> mDisplayProgram;
	int mWidth;
	int mHeight;
	uint32_t mFrameCount;
	CLCamera mGPUCamera;
	CPUCamera mCPUCamera;
};

RC_NAMESPACE_END
