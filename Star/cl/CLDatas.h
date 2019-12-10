#pragma once
#include "CLCore.h"

RC_NAMESPACE_BEGIN

struct Sphere
{
	cl_float radius;
	cl_float dummy1;
	cl_float dummy2;
	cl_float dummy3;
	cl_float3 position;
	cl_float3 color;
	cl_float3 emission;
};

struct Ray
{
	cl_float4 orig;
	cl_float4 dir;
};

struct Camera
{
	cl_float3 orig;
	cl_float3 front;
	cl_float3 up;
	// x : u fov, y : v fov, z : aperture , w : focusDist
	cl_float4 params;
};

struct HitData
{
	cl_int mask, objIndex, primIndex;
	cl_float t, u, v;
	cl_float2 rayID;
};

struct Environment
{
	cl_float4 envColAndClamp;
	cl_uint   envMap;
	cl_int    pad[3];
};

struct BVHNode
{
	cl_float3 bboxMin;
	cl_float3 bboxMax;
	cl_int numPrimitive;
	cl_int axis;
	cl_int primitiveOffset;
	cl_int secondChildOffset;
};

RC_NAMESPACE_END
