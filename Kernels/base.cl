__constant float EPSILON = 0.00003f;
__constant float PI = 3.14159265359f;
__constant int SAMPLES = 32;

#include "types.cl"
#include "bvh.cl"

Ray genCameraRay(const int x_coord, const int y_coord, const int width, const int height, __constant Camera* cam) 
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

__kernel void renderKernel(const int width, const int height, __constant Camera* cam, __global BVHNode* nodes, __global Triangle* tris, __write_only image2d_t output)
{
	unsigned int coordX = get_global_id(0);
	unsigned int coordY = get_global_id(1);


	Ray ray = genCameraRay(coordX, coordY, width, height, cam);
	float3 finalcolor = (float3)(0.0f, 0.0f, 0.0f);

	if(intersectBVH(&ray, nodes, tris))
	{
		finalcolor = (float3)(1.0f, 1.0f, 1.0f);
	}

	int2 coord=(int2)(coordX, coordY);
	float4 val = (float4)(finalcolor.x, finalcolor.y, finalcolor.z, 1.0);
    write_imagef(output, coord, val);

}