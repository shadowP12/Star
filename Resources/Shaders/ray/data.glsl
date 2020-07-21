struct Camera
{
    vec3 orig;
    vec3 front;
    vec3 up;
    vec4 params;
};

struct Ray
{
    vec3 origin;
    vec3 dir;
};

struct BVHNode
{
    vec3 bboxMin;
    vec3 bboxMax;
    int numPrimitive;
    int axis;
    int primitiveOffset;
    int secondChildOffset;
};

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec3 texcoord;
};

struct Material
{
    vec3 baseColor;
    vec3 emission;
    float metallic;
    float roughness;
};

struct Triangle
{
    Vertex v0;
    Vertex v1;
    Vertex v2;
    int mat;
};

struct IntersectData
{
    bool hit;
    Ray ray;
    float t;
    vec3 pos;
    vec3 texcoord;
    vec3 normal;
};