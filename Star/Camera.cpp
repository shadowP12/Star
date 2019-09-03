#include "Camera.h"

void Camera::tick()
{
	glm::vec2 offset = Input::instance().getMousePosition() - mLastPos;
	mLastPos = Input::instance().getMousePosition();
	if (Input::instance().getMouseButton(MouseButton::MouseRight))
	{
		mYaw += offset.x * 0.1f;
		mPitch += -offset.y * 0.1f;
		updateVectors();
		mView = glm::lookAt(mPos, mPos + mFront, mUp);
		mViewInv = glm::inverse(mView);
		mRasterToWorld = mViewInv * mRasterToCamera;
		mWorldToRaster = glm::inverse(mRasterToWorld);
	}
	if (Input::instance().getMouseScrollWheel() != 0)
	{
		float sw = Input::instance().getMouseScrollWheel();
		mPos += mFront * sw * 0.1f;
		mView = glm::lookAt(mPos, mPos + mFront, mUp);
		mViewInv = glm::inverse(mView);
		mRasterToWorld = mViewInv * mRasterToCamera;
		mWorldToRaster = glm::inverse(mRasterToWorld);
	}
}

void Camera::updateVectors()
{
	glm::vec3 front;
	front.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
	front.y = sin(glm::radians(mPitch));
	front.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
	mFront = glm::normalize(front);
	mRight = glm::normalize(glm::cross(mFront, glm::vec3(0, 1, 0)));
	mUp = glm::normalize(glm::cross(mRight, mFront));
}

void Camera::reSize(int w, int h)
{
	mWidth = w;
	mHeight = h;
	mRatio = mWidth / float(mHeight);
	mProj = glm::perspective(glm::radians(mFOV), mRatio, mNearClip, mFarClip);

	glm::mat4 s1;
	s1 = glm::scale(glm::mat4(1.0), glm::vec3(float(mWidth), float(mHeight), 1.0f));
	glm::mat4 s2;
	s2 = glm::scale(glm::mat4(1.0), glm::vec3(0.5f, -0.5f, 1.0f));
	glm::mat4 t;
	t = glm::translate(glm::mat4(1.0), glm::vec3(1.0f, -1.0f, 0.0f));
	mScreenToRaster =  s1 * s2 * t;
	
	mRasterToCamera = glm::inverse(mProj) * glm::inverse(mScreenToRaster);
	mCameraToRaster = glm::inverse(mRasterToCamera);
	mRasterToWorld = mViewInv * mRasterToCamera;
	mWorldToRaster = glm::inverse(mRasterToWorld);
	mScreenToWorld = mViewInv * glm::inverse(mProj);
}

bool Camera::GenerateRay(float s, float t, Ray& r)
{
	glm::mat4 viewProjectionMatrix = mProj * mView;
	glm::vec3 nearPoint = unProject(glm::vec4(0,0,mWidth,mHeight),glm::vec2(s,t),0.0,viewProjectionMatrix);
	glm::vec3 farPoint = unProject(glm::vec4(0, 0, mWidth, mHeight), glm::vec2(s, t), 1.0, viewProjectionMatrix);;
	glm::vec3 dir = farPoint - nearPoint;
	glm::normalize(dir);
	r.mOrig = nearPoint;
	r.mDir = dir;
	r.mMin = GMath::Epsilon;
	r.mMax = GMath::Infinity;
	/*
	glm::vec3 ScreenCoord;
	ScreenCoord.x = (2 * s) / (float)mWidth - 1.0;
	ScreenCoord.y = (2 * t) / (float)mHeight - 1.0;
	ScreenCoord.z = -0.05f;
	r.mOrig = mPos;
	r.mOrig.b *= -1;
	r.mDir = TransformVector(ScreenCoord, mScreenToWorld);
	r.mMin = GMath::Epsilon;
	r.mMax = GMath::Infinity;
	*/
	return true;
}
