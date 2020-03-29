#pragma OPENCL EXTENSION cl_nv_printf : enable
#define MAX_RENDER_DIST 200000.0f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define INV_PI 0.31830988618f
#define INV_TWO_PI 0.15915494309f
__constant float EPSILON = 0.00003f;
__constant int SAMPLES = 1;

#include "types.cl"
#include "tools.cl"

float intersectSphere(const Sphere* sphere, const Ray* ray)
{
	float3 rayToCenter = sphere->pos - ray->origin;
	float b = dot(rayToCenter, ray->dir);
	float c = dot(rayToCenter, rayToCenter) - sphere->radius*sphere->radius;
	float disc = b * b - c;

	if (disc < 0.0f) return 0.0f;
	else disc = sqrt(disc);

	if ((b - disc) > EPSILON) return b - disc;
	if ((b + disc) > EPSILON) return b + disc;
	
	return 0.0f;
}

bool occludedScene(Ray* ray, __global BVHNode* nodes, __global Triangle* tris)
{
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
				// leaf node
				for (int i = 0; i < node.numPrimitive; ++i)
				{
					int inx = node.primitiveOffset + i;
					if (occludedTriangle(ray, &tris[inx]))
					{
						return true;
					}
				}

				// miss
				if (todoOffset == 0)
					break;
				nodeNum = todo[--todoOffset];
			}
			else 
            {
					todo[todoOffset++] = node.secondChildOffset;
					nodeNum = nodeNum + 1;
			}
		}
		else
		{
			// miss
			if (todoOffset == 0)
				break;
			nodeNum = todo[--todoOffset];
		}
	}
	return false;
}

IntersectData intersectScene(Ray* ray, __global BVHNode* nodes, __global Triangle* tris)
{
	IntersectData isect;
    isect.hit = false;
    isect.ray = *ray;
    isect.t = MAX_RENDER_DIST;

	int todoOffset = 0;
    int nodeNum = 0;
	int todo[64];

	while (true)
	{
		BVHNode node = nodes[nodeNum];
		if (intersectBBox(ray->origin, ray->dir, node.bboxMin, node.bboxMax))
		{
			if (node.numPrimitive > 0)
			{

				// leaf node
				for (int i = 0; i < node.numPrimitive; ++i)
				{
					int inx = node.primitiveOffset + i;
					intersectTriangle(ray, &tris[inx], &isect);
				}

				// miss
				if (todoOffset == 0)
					break;
				nodeNum = todo[--todoOffset];
			}
			else 
            {
					todo[todoOffset++] = node.secondChildOffset;
					nodeNum = nodeNum + 1;
			}
		}
		else 
		{
			// miss
			if (todoOffset == 0)
				break;
			nodeNum = todo[--todoOffset];
		}
	}
	return isect;
}

Ray genCameraRay(const int width, const int height, const int x_coord, const int y_coord, __constant Camera* cam)
{
	// u v w
	float3 w = normalize(cam->front);
	float3 u = normalize(cross(cam->up, w));
    float3 v = cross(w, u);
	
	float aspect = (float)(width) / (float)(height);
	float theta = cam->params.x * PI / 180.0;
    float halfHeight = tan(theta/2);
    float halfWidth = aspect * halfHeight;
	float3 origin = cam->orig + cam->front;
	float3 lowerLeftCorner = origin - halfWidth*cam->params.w*u - halfHeight*cam->params.w*v - cam->params.w*w;
	float3 horizontal = 2*halfWidth*cam->params.w*u;
    float3 vertical = 2*halfHeight*cam->params.w*v;
	float3 pixelPos = lowerLeftCorner + (float)x_coord*horizontal + (float)y_coord*vertical;
	Ray ray;
	ray.origin = cam->orig;
	ray.dir = normalize(pixelPos - ray.origin);
	
	return ray;
}

Ray CreateRay(uint width, uint height, int coordX, int coordY, __constant Camera* cam)
{
    float3 cameraPos = cam->orig;
    float3 cameraFront = cam->front;
    float3 cameraUp = cam->up;

    float invWidth = 1.0f / (float)(width), invHeight = 1.0f / (float)(height);
    float aspectratio = (float)(width) / (float)(height);
    float fov = 45.0f * 3.1415f / 180.0f;
    float angle = tan(0.5f * fov);

    float x = coordX - 0.5f;
    float y = coordY - 0.5f;

    x = (2.0f * ((x + 0.5f) * invWidth) - 1) * angle * aspectratio;
    y = -(1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;

    float3 dir = normalize(x * cross(cameraFront, cameraUp) + y * cameraUp + cameraFront);
    Ray ray;
    ray.origin = cameraPos;
    ray.dir = dir;
    return ray;
}

float3 sampleHemisphereCosine(float3 n, unsigned int* seed0, unsigned int* seed1)
{
    // cos(theta) = r0 = y
    // cos^2(theta) + sin^2(theta) = 1 -> sin(theta) = srtf(1 - cos^2(theta)
    float r0 = getRandom(seed0, seed1);
    float r1 = getRandom(seed0, seed1);
    float sinTheta = sqrt(1.0 - r0 * r0);
    float phi = TWO_PI * r1;
    float x = sinTheta * cos(phi);
    float z = sinTheta * sin(phi);
    float y = r0;

    float3 nt = fabs(n.x) > fabs(n.y) ? (float3)(n.z, 0.0, -n.x)/sqrt(n.x*n.x + n.z*n.z) : (float3)(0.0, -n.z, n.y)/sqrt(n.y*n.y + n.z*n.z);
    float3 nb = cross(nt, n);

    float3 res = (float3)(
            x * nb.x + y * n.x + z * nt.x,
            x * nb.y + y * n.y + z * nt.y,
            x * nb.z + y * n.z + z * nt.z);

    return normalize(res);
}

float3 sampleDiffuse(float3 wo, float3* wi, float* pdf, float3 normal, __global Material* material, unsigned int* seed0, unsigned int* seed1)
{
    *wi = sampleHemisphereCosine(normal, seed0, seed1);
	*pdf = dot(*wi, normal) * INV_PI;
    return material->baseColor * INV_PI;
}

float3 sampleBrdf(float3 wo, float3* wi, float* pdf, float3 normal, __global Material* material, unsigned int* seed0, unsigned int* seed1)
{
	return sampleDiffuse(wo, wi, pdf, normal, material, seed0, seed1);
}

float3 Render(Ray* camray, __global BVHNode* nodes, __global Triangle* tris, __global Material* materials, unsigned int* seed0, unsigned int* seed1)
{
	float3 radiance = 0.0f;
    float3 beta = 1.0f;

	Ray ray = *camray;
	//return ray.origin;
	for (int i = 0; i < 10; ++i)
    {
        IntersectData isect = intersectScene(&ray, nodes, tris);
		
        if (!isect.hit)
        {
            break;
        }
        //return isect.normal;
		__global Material* material = &materials[isect.object->mat];

		radiance += beta * (material->emission) * 50.0f;
		float3 wo = -ray.dir;
		float3 wi;
		float pdf = 0.0f;
        float3 f = sampleBrdf(wo, &wi, &pdf, isect.normal, material, seed0, seed1);

        if (pdf <= 0.0f) break;
        float3 mul = f * dot(wi, isect.normal) / pdf;
        beta *= mul;

		ray.dir = wi;
		ray.origin = isect.pos + wi * 0.0001f;


		// Russian Roulette
		if(i > 3)
        {
            float r = getRandom(seed0, seed1);
            float rr = fmin(1.0f, luminance(radiance));
            if(r > rr)
                break;
            radiance /= rr;
        }
    }
    return max(radiance, 0.0f);
}

__kernel void renderKernel(const int width, const int height, __constant Camera* cam, __global BVHNode* nodes, __global Triangle* tris, __global Material* materials, unsigned int frameCount, __global float3* output)
{
	Sphere light;
	light.radius = 0.2f;
	light.pos = (float3)(0.0f, 1.3f, 0.0f);
	light.color = (float3)(0.0f, 0.0f, 0.0f);
	light.emission  = (float3)(9.0f, 8.0f, 7.0f);

	unsigned int coordX = get_global_id(0);
	unsigned int coordY = get_global_id(1);
	unsigned int seed0 = coordX + hashUInt32(frameCount);
	unsigned int seed1 = coordY + hashUInt32(frameCount);

	Ray ray = CreateRay(width, height, coordX, coordY, cam);
	float3 finalcolor = 0.0f;
	float invSamples = 1.0f / SAMPLES;
	for (int i = 0; i < SAMPLES; ++i)
	{
		finalcolor += Render(&ray, nodes, tris, materials, &seed0, &seed1) * invSamples;
	}

//	finalcolor = (float3)(clamp(finalcolor.x, 0.0f, 1.0f),
//						  clamp(finalcolor.y, 0.0f, 1.0f),
//						  clamp(finalcolor.z, 0.0f, 1.0f));

	// float t = intersectSphere(&light, &ray);
	// IntersectData isect = intersectScene(&ray, nodes, tris);
	// if(isect.hit || t != 0.0f)
	// {
	// 	finalcolor = (float3)(1.0f, 1.0f, 1.0f);
	// }

    output[coordY * width + coordX] = finalcolor;
}

__kernel void accumulate(__global float3* input, __global float3* output, uint width)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    float3 inputData = input[y * width + x];
    float3 outputData = output[y * width + x];
    output[y * width + x] = inputData + outputData;
}

__kernel void clearAccumulate(__global float3* data, uint width)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    data[y * width + x] = (float3)(0.0, 0.0, 0.0);
}

__kernel void display(__global float3* input, __write_only image2d_t output, uint sampleCount, uint width)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int2 texCoords = (int2)(x, y);
    float3 inputData = input[y * width + x];
    inputData = inputData / sampleCount;
    write_imagef(output, texCoords, (float4)(inputData, 1.0f));
}