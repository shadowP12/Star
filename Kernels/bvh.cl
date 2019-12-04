bool intersectBBOX(const float3 o, const float3 inv_d, const float t, const float3 bbox_min, const float3 bbox_max) 
{
    float low = inv_d.x * (bbox_min.x - o.x);
    float high = inv_d.x * (bbox_max.x - o.x);
    float tmin = fmin(low, high);
    float tmax = fmax(low, high);
    low = inv_d.y * (bbox_min.y - o.y);
    high = inv_d.y * (bbox_max.y - o.y);
    tmin = fmax(tmin, fmin(low, high));
    tmax = fmin(tmax, fmax(low, high));
    low = inv_d.z * (bbox_min.z - o.z);
    high = inv_d.z * (bbox_max.z - o.z);
    tmin = fmax(tmin, fmin(low, high));
    tmax = fmin(tmax, fmax(low, high));
    tmax *= 1.00000024f;
    return tmin <= tmax && tmin <= t && tmax > 0;
}

bool intersectBVH(const Ray* ray, const BVHNode* nodes) 
{
    while(1)
    {
        break;
    }
    return false;
}



