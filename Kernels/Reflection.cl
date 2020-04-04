float cosTheta(const float3 w)
{
	return w.z;
}

float cosTheta2(const float3 w)
{
	return w.z * w.z;
}

float absCosTheta(const float3 w)
{
	return fabs(w.z);
}

float sinTheta2(const float3 w)
{
	return max(0.0f, 1.0f - cosTheta2(w));
}

float sinTheta(const float3 w)
{
	return sqrt(sinTheta2(w));
}

float tanTheta(const float3 w)
{
	return sinTheta(w) / cosTheta(w);
}

float tanTheta2(const float3 w)
{
	return sinTheta2(w) / cosTheta2(w);
}

float cosPhi(const float3 w)
{
	float sint = sinTheta(w);
	if (sint == 0.0f)
	{
		return 1.0f;
	}
	return clamp(w.x / sint, -1.f, 1.f);
}

float cosPhi2(const float3 w)
{
	float cosphi = cosPhi(w);
	return cosphi * cosphi;
}

float sinPhi(const float3 w)
{
	float sint = sinTheta(w);
	if (sint == 0.0f)
	{
		return 0.0f;
	}
	return clamp(w.y / sint, -1.f, 1.f);
}

float sinPhi2(const float3 w)
{
	float sinphi = sinPhi(w);
	return sinphi * sinphi;
}

bool sameHemisphere(float3 w, float3 wp)
{
	return w.z * wp.z > 0.f;
}

float3 sampleLambertianReflection(float3 r, float3 wo, float3* wi, float* pdf, unsigned int* seed)
{
    *wi = cosineSampleHemisphere(seed);

    if (!sameHemisphere(wo, *wi))
    {
        wi->z *= -1.0f;
        *pdf = 0.0f;
    }
    else
    {
        *pdf = absCosTheta(*wi) * INV_PI;
    }
    return r * INV_PI;
}







