#pragma once
#include "GMath.h"
#include"Input/Input.h"
#include "Ray.h"
class Camera
{
public:
	Camera(const glm::vec3& pos, int w, int h, float fov, float nearClip, float farClip, float yaw = -90.0f, float pitch = 0.0f)
	{
		mPos = pos;
		mYaw = yaw;
		mPitch = pitch;
		mFront = glm::vec3(0, 0, -1);
		mUp = glm::vec3(0.0f, 1.0f, 0.0f);
		mNearClip = nearClip;
		mFarClip = farClip;
		mFOV = fov;
		
		mLastPos = Input::instance().getMousePosition();
		updateVectors();
		mView = glm::lookAt(mPos, mFront, mUp);
		mViewInv = glm::inverse(mView);
		reSize(w, h);
	}

	void tick();
	void reSize(int w, int h);
	bool GenerateRay(float s, float t, Ray& r);
	int getWidth(){ return mWidth; }
	int getHeight(){ return mHeight; }
	glm::mat4 getViewMatrix(){ return mView; }
	glm::mat4 getProjMatrix() { return mProj; }
	bool CheckRaster(const glm::vec3& pRas){ return pRas.x < float(mWidth) && pRas.x >= 0.0f && pRas.y < float(mHeight) && pRas.y >= 0.0f; }
	glm::vec3 getFront() { return mFront; }
	glm::vec3 getUp() { return mUp; }
	glm::vec3 getRight() { return mRight; }
private:
	void updateVectors();
private:
	glm::vec3 mPos;
	glm::vec3 mFront;
	glm::vec3 mUp;
	glm::vec3 mRight;

	int mWidth;
	int mHeight;

	float mYaw;
	float mPitch;
	float mNearClip;
	float mFarClip;
	float mFOV;
	float mRatio;

	glm::vec2 mLastPos;

	glm::mat4 mView;
	glm::mat4 mViewInv;
	glm::mat4 mProj;
};
