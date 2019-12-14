#pragma once
#include "Triangle.h"
#include "BBox.h"
#include "Intersection.h"
#include "Ray.h"
class Primitive
{
public:
	Primitive(std::shared_ptr<Triangle> tri)
	{
		mTriangle = tri;
	}
	~Primitive() {}
	BBox worldBound();
	bool intersect(Ray& r, Intersection& in);
	bool intersectP(Ray& r);
	std::shared_ptr<Triangle> getTri() { return mTriangle; }
private:
	std::shared_ptr<Triangle> mTriangle;
};