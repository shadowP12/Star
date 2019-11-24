#pragma once
#include <vector>

struct ShapeDesc
{
	ShapeDesc(size_t inStart, size_t inCount)
		:start(inStart), count(inCount)
	{}
	size_t start;
	size_t count;
};

struct MeshDesc
{
	float* vertices;
	size_t verticesCount;
	uint32_t* indices;
	size_t indicesCount; 
	std::vector<ShapeDesc> shapes;
};

enum LightType 
{
	LT_POINT_LIGHT,
	LT_SPOT_LIGHT,
	LT_DIRECTIONAL_LIGHT
};

struct LightDesc 
{
	float position[3];
	float radius;
	float color[3];
	float direction[3];
	float angle;
	LightType type;
};

struct CameraDesc 
{
	float orig[3];
	float fwd[3];
	float up[3];
	float fov;
};