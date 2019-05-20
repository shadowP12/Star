#pragma once
#include "Triangle.h"
#include "BBox.h"
class Primitive
{
public:
	Primitive(std::shared_ptr<Triangle> tri)
	{
		mTriangle = tri;
	}
	~Primitive() {}
	BBox worldBound();
private:
	std::shared_ptr<Triangle> mTriangle;
};