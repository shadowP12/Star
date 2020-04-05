// occluded triangle 
bool occludedTriangle(Ray* ray, __global Triangle* triangle)
{
	float3 e1 = triangle->v1.position - triangle->v0.position;
	float3 e2 = triangle->v2.position - triangle->v0.position;
	float3 p = cross(ray->dir, e2);

	float a = dot(e1, p);

	if(fabs(a) < 0.0001)
		return false;
	float f = 1.0f/a;
	float3 s = ray->origin - triangle->v0.position;
	float u = f*dot(s, p);

	if(u < 0.0 || u > 1.0)
		return false;
	float3 q = cross(s, e1);
	float v = f*dot(ray->dir, q);
	if(v < 0.0 || (u + v) > 1.0)
		return false;
	return true;
}

// intersect triangle 
bool intersectTriangle(Ray* ray, __global Triangle* triangle, IntersectData* isect)
{
	float3 e1 = triangle->v1.position - triangle->v0.position;
	float3 e2 = triangle->v2.position - triangle->v0.position;
	float3 p = cross(ray->dir, e2);

	float a = dot(e1, p);

	if(fabs(a) < 0.0001)
		return false;
	float f = 1.0/a;
	float3 s = ray->origin - triangle->v0.position;
	float u = f*dot(s, p);

	if(u < 0.0 || u > 1.0)
		return false;
	float3 q = cross(s, e1);
	float v = f*dot(ray->dir, q);
	if(v < 0.0 || (u + v) > 1.0)
		return false;

	float t = dot(e2, q) * f;

	if(t > 0.0f && t < isect->t)
	{
		isect->hit = true;
        isect->t = t;
        isect->pos = isect->ray.origin + isect->ray.dir * t;
        isect->object = triangle;
        isect->normal = normalize(u * triangle->v1.normal + v * triangle->v2.normal + (1.0f - u - v) * triangle->v0.normal);
        isect->texcoord = u * triangle->v1.texcoord + v * triangle->v2.texcoord + (1.0f - u - v) * triangle->v0.texcoord;
        return true;
	}
	return false;
}

// orig : 光线的起点
// dir : 光线的方向 
// min : 包围盒最小点
// max : 包围盒最大点
bool intersectBBox(const float3 orig, const float3 dir, const float3 min, const float3 max) 
{
    return true;
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

float2 pointInHexagon(unsigned int* seed)
{
    float2 hexPoints[3] = { (float2)(-1.0f, 0.0f), (float2)(0.5f, 0.866f), (float2)(0.5f, -0.866f) };
    int x = floor(getRandomFloat(seed) * 3.0f);
    float2 v1 = hexPoints[x];
    float2 v2 = hexPoints[(x + 1) % 3];
    float p1 = getRandomFloat(seed);
    float p2 = getRandomFloat(seed);
    return (float2)(p1 * v1.x + p2 * v2.x, p1 * v1.y + p2 * v2.y);
}

unsigned int hashUInt32(unsigned int x)
{
    return 1103515245 * x + 12345;
}

float3 randomInUnitDisk(unsigned int* seed)
{
    float3 p;
    do
    {
        p = 2.0f * (float3)(getRandomFloat(seed), getRandomFloat(seed), 0) - (float3)(1,1,0);
    } while (dot(p,p) >= 1.0);
    return p;
}

float3 randomInUnitSphere(unsigned int* seed)
{
    float3 p;
    do
    {
        p = 2.0f * (float3)(getRandomFloat(seed), getRandomFloat(seed), getRandomFloat(seed)) - (float3)(1,1,1);
    } while (dot(p,p) >= 1.0);
    return p;
}

float2 rejectionSampleDisk(unsigned int* seed)
{
	float x,y;
	do {
		x = getRandomFloat(seed);
		y = getRandomFloat(seed);
	} while (x * x + y * y > 1.0f);
	return (float2)(x, y);
}


