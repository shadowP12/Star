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
