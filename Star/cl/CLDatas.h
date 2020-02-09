#pragma once
#include "CLCore.h"

RC_NAMESPACE_BEGIN

struct CLSphere
{
	cl_float radius;
	cl_float dummy1;
	cl_float dummy2;
	cl_float dummy3;
	cl_float3 position;
	cl_float3 color;
	cl_float3 emission;
};

struct CLRay
{
	cl_float4 orig;
	cl_float4 dir;
};

struct CLCamera
{
	cl_float3 orig;
	cl_float3 front;
	cl_float3 up;
	// x : u fov, y : v fov, z : aperture , w : focusDist
	cl_float4 params;
};

struct CLHitData
{
	cl_int mask, objIndex, primIndex;
	cl_float t, u, v;
	cl_float2 rayID;
};

struct CLEnvironment
{
	cl_float4 envColAndClamp;
	cl_uint   envMap;
	cl_int    pad[3];
};

struct CLBVHNode
{
	cl_float3 bboxMin;
	cl_float3 bboxMax;
	cl_int numPrimitive;
	cl_int axis;
	cl_int primitiveOffset;
	cl_int secondChildOffset;
};

struct CLVertex
{
    cl_float3 position;
    cl_float3 normal;
    cl_float3 texcoord;
};

struct CLTriangle
{
    CLVertex v0;
    CLVertex v1;
    CLVertex v2;
    cl_int mat;
    cl_int pad[3];
};

struct CLMaterial
{
    cl_float3 baseColor;
    cl_float3 emissive;
};

RC_NAMESPACE_END
