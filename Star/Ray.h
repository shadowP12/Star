#pragma once
#include "GMath.h"
#include <assert.h>

struct Ray
{
	Ray() {}
	Ray(glm::vec3 orig, glm::vec3 dir, float min = 0, float max = GMath::Infinity)
		: mOrig(orig), mDir(dir), mMin(min), mMax(max)
	{
		//这里必须保证方向归一化
	}

	glm::vec3 pointAt(float t) const { return mOrig + mDir * t; }

	glm::vec3 mOrig;
	glm::vec3 mDir;
	float mMin;
	float mMax;
};