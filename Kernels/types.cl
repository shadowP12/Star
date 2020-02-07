typedef struct Camera {
	float3 orig;
	float3 front;
    float3 up;
    float4 params;
} Camera;

typedef struct Ray{
	float3 origin;
	float3 dir;
} Ray;

typedef struct Sphere{
	float radius;
	float3 pos;
	float3 color;
	float3 emission;
} Sphere;

typedef struct BVHNode{
	float3 bboxMin;
	float3 bboxMax;
	int numPrimitive;
	int axis;
	int primitiveOffset;
	int secondChildOffset;
} BVHNode;

typedef struct Vertex
{
    float3 position;
    float3 normal;
    float3 texcoord;
} Vertex;

typedef struct Triangle{
	Vertex v0;
	Vertex v1;
	Vertex v2;
	int mat;
} Triangle;

typedef struct IntersectData {
    bool hit;
    Ray ray;
    float t;
    float3 pos;
    float3 texcoord;
    float3 normal;
    const __global Triangle* object;
} IntersectData;

typedef struct Scene {
    __global Triangle* triangles;
    __global BVHNode* nodes;
} Scene;