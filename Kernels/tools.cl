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

	if(t < isect->t)
	{
		isect->hit = true;
        isect->t = t;
        isect->pos = isect->ray.origin + isect->ray.dir * t;
        isect->object = triangle;
        isect->normal = normalize(u * triangle->v1.normal + v * triangle->v2.normal + (1.0f - u - v) * triangle->v0.normal);
        isect->texcoord = u * triangle->v1.texcoord + v * triangle->v2.texcoord + (1.0f - u - v) * triangle->v0.texcoord;
	}
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

float3 saturate(float3 value)
{
    return min(max(value, 0.0f), 1.0f);
}

float getRandomFloat(unsigned int* seed)
{
    *seed = (*seed ^ 61) ^ (*seed >> 16);
    *seed = *seed + (*seed << 3);
    *seed = *seed ^ (*seed >> 4);
    *seed = *seed * 0x27d4eb2d;
    *seed = *seed ^ (*seed >> 15);
    *seed = 1103515245 * (*seed) + 12345;

    return (float)(*seed) * 2.3283064365386963e-10f;
}

float getRandom(unsigned int *seed0, unsigned int *seed1)
{
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	unsigned int ires = ((*seed0) << 16) + (*seed1);

	union {
		float f;
		unsigned int ui;
	} res;

	res.ui = (ires & 0x007fffff) | 0x40000000;
	return (res.f - 2.0f) / 2.0f;
}

unsigned int hashUInt32(unsigned int x)
{
    return 1103515245 * x + 12345;
}

float3 reflect(float3 v, float3 n)
{
    return -v + 2.0f * dot(v, n) * n;
}

float3 sampleHemisphereCosine(float3 n, unsigned int* seed0, unsigned int* seed1)
{
    float phi = TWO_PI * getRandom(seed0, seed1);
    float sinThetaSqr = getRandom(seed0, seed1);
    float sinTheta = sqrt(sinThetaSqr);

    float3 axis = fabs(n.x) > 0.001f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
    float3 t = normalize(cross(axis, n));
    float3 s = cross(n, t);

    return normalize(s*cos(phi)*sinTheta + t*sin(phi)*sinTheta + n*sqrt(1.0f - sinThetaSqr));
}

float2 pointInHexagon(unsigned int* seed0, unsigned int* seed1)
{
    float2 hexPoints[3] = { (float2)(-1.0f, 0.0f), (float2)(0.5f, 0.866f), (float2)(0.5f, -0.866f) };
    int x = floor(getRandom(seed0, seed1) * 3.0f);
    float2 v1 = hexPoints[x];
    float2 v2 = hexPoints[(x + 1) % 3];
    float p1 = getRandom(seed0, seed1);
    float p2 = getRandom(seed0, seed1);
    return (float2)(p1 * v1.x + p2 * v2.x, p1 * v1.y + p2 * v2.y);
}

float3 randomInUnitDisk(unsigned int* seed0, unsigned int* seed1)
{
    float3 p;
    do
    {
        p = 2.0f * (float3)(getRandom(seed0, seed1), getRandom(seed0, seed1), 0) - (float3)(1,1,0);
    } while (dot(p,p) >= 1.0);
    return p;
}

float3 randomInUnitSphere(unsigned int* seed0, unsigned int* seed1)
{
    float3 p;
    do
    {
        p = 2.0f * (float3)(getRandom(seed0, seed1), getRandom(seed0, seed1), getRandom(seed0, seed1)) - (float3)(1,1,1);
    } while (dot(p,p) >= 1.0);
    return p;
}

float3 randomUnitVector(unsigned int* seed0, unsigned int* seed1)
{
    float z = getRandom(seed0, seed1) * 2.0f - 1.0f;
    float a = getRandom(seed0, seed1) * 2.0f * PI;
    float r = sqrt(1.0f - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return (float3)(x, y, z);
}

float2 rejectionSampleDisk(unsigned int* seed0, unsigned int* seed1)
{
	float x,y;
	do {
		x = getRandom(seed0, seed1);
		y = getRandom(seed0, seed1);
	} while (x * x + y * y > 1.0f);
	return (float2)(x, y);
}

bool sameHemisphere(float3 w, float3 wp)
{
	return w.z * wp.z > 0.f;
}

float3 uniformSampleHemisphere(unsigned int* seed0, unsigned int* seed1)
{
	float ux = getRandom(seed0, seed1);
	float uy = getRandom(seed0, seed1);
	float z = ux;
	float r = sqrt(1.0f - z*z);
	float phi = 2*PI*uy;
	float x = r * cos(phi);
    float y = r * sin(phi);

    return (float3)(x, y, z);
}



