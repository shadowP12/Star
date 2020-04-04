#include "Common.cl"
#include "Types.cl"
#include "Sampling.cl"
#include "Reflection.cl"
#include "Tools.cl"

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

Ray createRay(uint width, uint height, float3 cameraPos, float3 cameraFront, float3 cameraUp, unsigned int* seed)
{
    float invWidth = 1.0f / (float)(width), invHeight = 1.0f / (float)(height);
    float aspectratio = (float)(width) / (float)(height);
    float fov = 45.0f * 3.1415f / 180.0f;
    float angle = tan(0.5f * fov);

    float x = (float)(get_global_id(0) % width) + getRandomFloat(seed) - 0.5f;
    float y = (float)(get_global_id(0) / width) + getRandomFloat(seed) - 0.5f;

    x = (2.0f * ((x + 0.5f) * invWidth) - 1) * angle * aspectratio;
    y = -(1.0f - 2.0f * ((y + 0.5f) * invHeight)) * angle;

    float3 dir = normalize(x * cross(cameraFront, cameraUp) + y * cameraUp + cameraFront);

    Ray ray;
    ray.origin = cameraPos;
    ray.dir = dir;
    return ray;
}

float3 render(Ray* camray, __global BVHNode* nodes, __global Triangle* tris, __global Material* materials, unsigned int* seed)
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
		__global Material* material = &materials[isect.object->mat];

        // Local Coordinate
        float3 n = isect.normal;
        float3 t = fabs(n.x) > fabs(n.y) ? (float3)(n.z, 0.0, -n.x)/sqrt(n.x*n.x + n.z*n.z) : (float3)(0.0, -n.z, n.y)/sqrt(n.y*n.y + n.z*n.z);
        float3 b = cross(t, n);

		radiance += beta * (material->emission) * 50.0f;
		//float3 wo = -ray.dir;
		float3 wo = worldToLocal(b , t, n, -ray.dir);
		float3 wi;
		float pdf = 0.0f;
		float3 f = sampleLambertianReflection(material->baseColor, wo, &wi, &pdf, seed);
        if (pdf <= 0.0f) break;
        wi = localToWorld(b, t, n, wi);
        float3 mul = f * dot(wi, n) / pdf;
        beta *= mul;

		ray.dir = wi;
		ray.origin = isect.pos + wi * 0.0001f;

		// Russian Roulette
		if(i > 3)
        {
            float r = getRandomFloat(seed);
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

	unsigned int seed = get_global_id(0) + hashUInt32(frameCount);

	Ray ray = createRay(width, height, cam->orig, cam->front, cam->up, &seed);
	float3 finalcolor = 0.0f;
	float invSamples = 1.0f / SAMPLES;
	for (int i = 0; i < SAMPLES; ++i)
	{
		finalcolor += render(&ray, nodes, tris, materials, &seed) * invSamples;
	}

	// float t = intersectSphere(&light, &ray);
	// IntersectData isect = intersectScene(&ray, nodes, tris);
	// if(isect.hit || t != 0.0f)
	// {
	// 	finalcolor = (float3)(1.0f, 1.0f, 1.0f);
	// }

    output[get_global_id(0)] = finalcolor;
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