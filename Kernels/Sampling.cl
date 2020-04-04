
float3 cosineSampleHemisphere(unsigned int* seed)
{
    // cos(theta) = r0 = y
    // cos^2(theta) + sin^2(theta) = 1 -> sin(theta) = srtf(1 - cos^2(theta)
    float r0 = getRandomFloat(seed);
    float r1 = getRandomFloat(seed);
    float sinTheta = sqrt(1.0 - r0 * r0);
    float phi = TWO_PI * r1;
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = r0;

    return (float3)(x, y, z);
}