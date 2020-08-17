#include "BBox.h"
static float gInfinity = std::numeric_limits<float>::infinity();
static float gNegInfinity = -gInfinity;
static float gEpsilon = std::numeric_limits<float>::epsilon();

namespace accel {
    BBox::BBox()
    {
        mMin = glm::vec3(gInfinity, gInfinity, gInfinity);
        mMax = glm::vec3(gNegInfinity, gNegInfinity, gNegInfinity);
    }

    BBox::BBox(const glm::vec3& p)
    {
        mMin = p;
        mMax = p;
    }

    BBox::BBox(const glm::vec3& p1, const glm::vec3& p2)
    {
        mMin = glm::vec3(glm::min(p1.x, p2.x), glm::min(p1.y, p2.y), glm::min(p1.z, p2.z));
        mMax = glm::vec3(glm::max(p1.x, p2.x), glm::max(p1.y, p2.y), glm::max(p1.z, p2.z));
    }

    glm::vec3 BBox::center()
    {
        return (mMax + mMin) * 0.5f;
    }

    glm::vec3 BBox::diagonal()
    {
        return (mMax - mMin);
    }

    float BBox::surfaceArea()
    {
        glm::vec3 d = diagonal();
        return (d.x * d.y + d.x * d.z + d.y * d.z) * 2;
    }

    float BBox::volume()
    {
        glm::vec3 d = diagonal();
        return d.x * d.y * d.z;
    }

    int BBox::maximumExtent()
    {
        glm::vec3 diag = diagonal();
        if (diag.x > diag.y && diag.x > diag.z)
        {
            return 0;
        }
        else if (diag.y > diag.z)
        {
            return 1;
        }
        else
        {
            return 2;
        }
    }

    void BBox::grow(const BBox &bbox)
    {
        mMin.x = glm::min(mMin.x, bbox.mMin.x);
        mMin.y = glm::min(mMin.y, bbox.mMin.y);
        mMin.z = glm::min(mMin.z, bbox.mMin.z);
        mMax.x = glm::max(mMax.x, bbox.mMax.x);
        mMax.y = glm::max(mMax.y, bbox.mMax.y);
        mMax.z = glm::max(mMax.z, bbox.mMax.z);
    }

    void BBox::grow(const glm::vec3& point)
    {
        mMin.x = glm::min(mMin.x, point.x);
        mMin.y = glm::min(mMin.y, point.y);
        mMin.z = glm::min(mMin.z, point.z);
        mMax.x = glm::max(mMax.x, point.x);
        mMax.y = glm::max(mMax.y, point.y);
        mMax.z = glm::max(mMax.z, point.z);
    }

    BBox BBox::grow(const BBox &bbox1, const BBox &bbox2)
    {
        BBox ret;

        ret.mMin.x = glm::min(bbox1.mMin.x, bbox2.mMin.x);
        ret.mMin.y = glm::min(bbox1.mMin.y, bbox2.mMin.y);
        ret.mMin.z = glm::min(bbox1.mMin.z, bbox2.mMin.z);
        ret.mMax.x = glm::max(bbox1.mMax.x, bbox2.mMax.x);
        ret.mMax.y = glm::max(bbox1.mMax.y, bbox2.mMax.y);
        ret.mMax.z = glm::max(bbox1.mMax.z, bbox2.mMax.z);

        return ret;
    }

    BBox BBox::grow(const BBox &bbox, const glm::vec3 &point)
    {
        BBox ret;

        ret.mMin.x = glm::min(bbox.mMin.x, point.x);
        ret.mMin.y = glm::min(bbox.mMin.y, point.y);
        ret.mMin.z = glm::min(bbox.mMin.z, point.z);
        ret.mMax.x = glm::max(bbox.mMax.x, point.x);
        ret.mMax.y = glm::max(bbox.mMax.y, point.y);
        ret.mMax.z = glm::max(bbox.mMax.z, point.z);

        return ret;
    }
}
