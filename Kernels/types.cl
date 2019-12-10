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

typedef struct Triangle{
	float3 p0;
	float3 p1;
	float3 p2;
} Triangle;