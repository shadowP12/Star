#include "BVH.h"
bool boxIntersect(const BBox &bounds, const Ray &ray,
	const glm::vec3 &invDir, const unsigned int dirIsNeg[3]) 
{

	float tmin = (bounds[dirIsNeg[0]].x - ray.mOrig.x) * invDir.x;
	float tmax = (bounds[1 - dirIsNeg[0]].x - ray.mOrig.x) * invDir.x;
	float tymin = (bounds[dirIsNeg[1]].y - ray.mOrig.y) * invDir.y;
	float tymax = (bounds[1 - dirIsNeg[1]].y - ray.mOrig.y) * invDir.y;
	if ((tmin > tymax) || (tymin > tmax))
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	float tzmin = (bounds[dirIsNeg[2]].z - ray.mOrig.z) * invDir.z;
	float tzmax = (bounds[1 - dirIsNeg[2]].z - ray.mOrig.z) * invDir.z;
	if ((tmin > tzmax) || (tzmin > tmax))
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	return (tmin < ray.mMax) && (tmax > ray.mMin);
}

bool BVH::intersect(Ray& ray, Intersection& in)
{
	if (!mNodes)
		return false;
	bool hit = false;
	glm::vec3 invDir(1.0f / ray.mDir.x, 1.0f / ray.mDir.y, 1.0f / ray.mDir.z);
	unsigned int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };
	unsigned int todoOffset = 0, nodeNum = 0;
	unsigned int todo[64];

	while (true) 
	{
		const LinearBVHNode *node = &mNodes[nodeNum];
		if (boxIntersect(node->bound, ray, invDir, dirIsNeg)) 
		{
			if (node->numPrimitive > 0) 
			{
				for (unsigned int i = 0; i < node->numPrimitive; ++i) 
				{
					if (mPrimitives[node->primitiveOffset + i]->intersect(ray,in)) 
					{
						hit = true;
					}
				}

				if (todoOffset == 0)
					break;
				nodeNum = todo[--todoOffset];
			}
			else 
			{
				if (dirIsNeg[node->axis]) 
				{
					todo[todoOffset++] = nodeNum + 1;
					nodeNum = node->secondChildOffset;
				}
				else 
				{
					todo[todoOffset++] = node->secondChildOffset;
					nodeNum = nodeNum + 1;
				}
			}
		}
		else 
		{
			if (todoOffset == 0)
				break;
			nodeNum = todo[--todoOffset];
		}
	}
	return hit;
}
bool BVH::intersectP(Ray& ray)
{
	if (!mNodes)
		return false;
	glm::vec3 invDir(1.0f / ray.mDir.x, 1.0f / ray.mDir.y, 1.0f / ray.mDir.z);
	unsigned int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };
	unsigned int todoOffset = 0, nodeNum = 0;
	unsigned int todo[64];

	while (true) 
	{
		const LinearBVHNode *node = &mNodes[nodeNum];
		if (boxIntersect(node->bound, ray, invDir, dirIsNeg))
		{
			if (node->numPrimitive > 0) 
			{
				//节点中有图元
				for (unsigned int i = 0; i < node->numPrimitive; ++i) 
				{
					if (mPrimitives[node->primitiveOffset + i]->intersectP(ray)) 
					{
						return true;
					}
				}

				if (todoOffset == 0)
					break;
				nodeNum = todo[--todoOffset];
			}
			else {
				if (dirIsNeg[node->axis]) 
				{
					//先遍历第二个子节点
					todo[todoOffset++] = nodeNum + 1;
					nodeNum = node->secondChildOffset;
				}
				else 
				{
					todo[todoOffset++] = node->secondChildOffset;
					nodeNum = nodeNum + 1;
				}
			}
		}
		else 
		{
			//没射中node
			if (todoOffset == 0)
				break;
			nodeNum = todo[--todoOffset];
		}
	}
	return false;
}