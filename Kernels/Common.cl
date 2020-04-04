#define MAX_RENDER_DIST 200000.0f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define INV_PI 0.31830988618f
#define INV_TWO_PI 0.15915494309f
#define EPSILON 0.00003f
#define SAMPLES 1

float3 saturate(float3 value)
{
    return min(max(value, 0.0f), 1.0f);
}

float luminance(float3 color)
{
    return color.x * 0.212671 + color.y * 0.715160 + color.z * 0.072169;
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

float3 localToWorld(float3 b, float3 t, float3 n, float3 w)
{
	return b * w.x + t * w.y + n * w.z;
}

float3 worldToLocal(float3 b, float3 t, float3 n, float3 w)
{
	return (float3)(dot(w, b), dot(w, t), dot(w, n));
}