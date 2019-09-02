#include "Primitive.h"

BBox Primitive::worldBound()
{
	return mTriangle->worldBound();
}

bool Primitive::intersect(Ray & r, Intersection & in)
{
	bool ret = mTriangle->intersect(r,in);
	if (ret)
	{
		in.mPrimitive = this;
		return true;
	}
	return false;
}

bool Primitive::intersectP(Ray & r)
{
	return mTriangle->intersectP(r);
}
