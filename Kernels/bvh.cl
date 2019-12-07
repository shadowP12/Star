
// orig : 光线的起点
// dir : 光线的方向 
// min : 包围盒最小点
// max : 包围盒最大点
bool intersect(const float3 orig, const float3 dir, const float3 min, const float3 max) 
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

bool intersectBVH(const Ray* ray, __constant BVHNode* nodes) 
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
		if (intersect(ray->origin, ray->dir, node.bboxMin, node.bboxMax))
		{
			if (node.numPrimitive > 0) 
			{
				// 叶子节点
				// 节点中有图元
				return true;

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



