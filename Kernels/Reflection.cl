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

float3 reflect(float3 v, float3 n)
{
    return -v + 2.0f * dot(v, n) * n;
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

float GGX_D(float3 wh, float alpha)
{
	if (wh.z <= 0.0f)
		return 0.0f;

	float _tanTheta2 = tanTheta2(wh);
	float _cosTheta2 = cosTheta2(wh);

    float root = alpha / (_cosTheta2 * (alpha * alpha + _tanTheta2));

	return INV_PI * (root * root);
}

float smithG(float3 v, float3 wh, float alpha)
{
	float _tanTheta = fabs(tanTheta(v));

	if (_tanTheta == 0.0f)
		return 1.0f;

	if (dot(v, wh) * cosTheta(v) <= 0)
		return 0.0f;

	float root = alpha * _tanTheta;
	return 2.0f / (1.0f + sqrt(1.0f + root*root));
}

float GGX_G(float3 wo, float3 wi, float3 wh, float alpha)
{
	return smithG(wo, wh, alpha) * smithG(wi, wh, alpha);
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float3 sampleMicrofacetReflection(float3 R, float alpha, float3 wo, float3* wi, float* pdf, unsigned int* seed)
{
    float r0 = getRandomFloat(seed);
    float r1 = getRandomFloat(seed);

    float phi = r0 * 2 * PI;
    float _cosTheta = sqrt((1.0f - r1) / (1.0f + r1 * (alpha * alpha - 1.0f)));
    float _sinTheta = sqrt(1.0f - (_cosTheta * _cosTheta));
    float x = _sinTheta * cos(phi);
    float y = _sinTheta * sin(phi);
    float z = _cosTheta;
    float3 wh = (float3)(x, y, z);
    if(cosTheta(wo) < 0.0f)
    {
        wh = -wh;
    }
    wh = normalize(wh);
    *wi = reflect(wo, wh);
    if (!sameHemisphere(wo, *wi))
    {
        *pdf = 0.0f;
        return (float3)(0.0f);
    }

    // pdf
    float pdf_wh = GGX_D(wh, alpha) * absCosTheta(wh);
    *pdf = pdf_wh / (4 * dot(wo, wh));

    // f
    float _cosThetaWo = absCosTheta(wo);
    float _cosThetaWi = absCosTheta(*wi);
    float3 F = fresnelSchlick(dot(*wi, wh), R);
    float G = GGX_G(wo, *wi, wh, alpha);
    float D = GGX_D(wh, alpha);

    return R * (F * D * G) / (4 * _cosThetaWo * _cosThetaWi);
}









