#pragma once
#include "GMath.h"

class Primitive;
struct Intersection
{
	Primitive* mPrimitive;
	glm::vec3 mPos;
	glm::vec3 mNormal;
	float mDistance;
};
