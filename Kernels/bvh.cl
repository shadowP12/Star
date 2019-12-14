bool intersectTri(const float3 orig, const float3 dir, const float3 p0, const float3 p1, const float3 p2)
{
	float3 e1 = p1 - p0;
	float3 e2 = p2 - p0;
	float3 p = cross(dir, e2);

	float a = dot(e1, p);

	if(fabs(a) < 0.0001)
		return false;
	float f = 1.0/a;
	float3 s = orig - p0;
	float u = f*dot(s, p);

	if(u < 0.0 || u > 1.0)
		return false;
	float3 q = cross(s, e1);
	float v = f*dot(dir, q);
	if(v < 0.0 || (u + v) > 1.0)
		return false;
	return true;
}


// orig : 光线的起点
// dir : 光线的方向 
// min : 包围盒最小点
// max : 包围盒最大点
bool intersectBBox(const float3 orig, const float3 dir, const float3 min, const float3 max) 
{
	float low = (min.x - orig.x)/dir.x;
	float high = (max.x - orig.x)/dir.x;
	float txmin =  fmin(low, high);
	float txmax = fmax(low, high);

	low = (min.y - orig.y)/dir.y;
	high = (max.y - orig.y)/dir.y;
	float tymin =  fmin(low, high);
	float tymax = fmax(low, high);

	if ((txmin > tymax) || (tymin > txmax))
		return false;

	low = (min.z - orig.z)/dir.z;
	high = (max.z - orig.z)/dir.z;
	float tzmin =  fmin(low, high);
	float tzmax = fmax(low, high);

	if ((txmin > tzmax) || (tzmin > txmax))
		return false;

	return true;
}

bool intersectBVH(const Ray* ray, __global BVHNode* nodes, __global Triangle* tris)
{
	float3 invDir = (float3)(1.0 / ray->dir.x, 1.0 / ray->dir.y, 1.0 / ray->dir.z);
    int dirIsNeg[3];
    dirIsNeg[0] = invDir.x < 0 ? 1:0;
    dirIsNeg[1] = invDir.y < 0 ? 1:0;
    dirIsNeg[2] = invDir.z < 0 ? 1:0;
	int todoOffset = 0;
    int nodeNum = 0;
	int todo[128];

	while (true) 
	{
		BVHNode node = nodes[nodeNum];
		if (intersectBBox(ray->origin, ray->dir, node.bboxMin, node.bboxMax))
		{
			if (node.numPrimitive > 0) 
			{
				// 叶子节点
				// 节点中有图元
				for (int i = 0; i < node.numPrimitive; ++i) 
				{
					int inx = node.primitiveOffset + i;
					Triangle tri = tris[inx];
					float3 t0 = tri.p0;
					float3 t1 = tri.p1;
					float3 t2 = tri.p2;
					if (intersectTri(ray->origin, ray->dir, t0, t1, t2)) 
					{
						return true;
					}
				}

				if (todoOffset == 0)
					break;
				nodeNum = todo[--todoOffset];
			}
			else 
            {
				// 中间节点
				if (dirIsNeg[node.axis]) 
				{
					//先遍历第二个子节点
					todo[todoOffset++] = nodeNum + 1;
					nodeNum = node.secondChildOffset;
				}
				else 
				{
					todo[todoOffset++] = node.secondChildOffset;
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



