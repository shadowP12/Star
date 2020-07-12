#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <math.h>

static constexpr float Infinity = std::numeric_limits<float>::infinity();
static constexpr float NegInfinity = -Infinity;
static constexpr float Epsilon = std::numeric_limits<float>::epsilon();
static constexpr float Pi = 3.14159265358979323846;
static constexpr float InvPi = 1.0f / Pi;
static constexpr float TwoPi = 2.0f * Pi;
static constexpr float InvTwoPi = 1.0f / TwoPi;

inline glm::vec3 TransformPoint(const glm::vec3& point, const glm::mat4& inMat)
{
//    glm::vec4 p = glm::vec4(point.x, point.y, point.z, 1.0f);
//    glm::vec4 r = inMat * p;
//    return glm::vec3(r.x, r.y, r.z);
    glm::mat4 mat = glm::transpose(inMat);
    float x = point.x, y = point.y, z = point.z;
    float xp = mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3];
    float yp = mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3];
    float zp = mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3];
    float wp = mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3];
    assert(wp != 0);

    if (wp == 1.0f)
        return glm::vec3(xp, yp, zp);
    else
        return glm::vec3(xp, yp, zp) / wp;
}

inline glm::vec4 TransformPoint(const glm::vec4& point, const glm::mat4& inMat)
{
    glm::mat4 mat = glm::transpose(inMat);
    float x = point.x, y = point.y, z = point.z, w = point.w;
    float xp = mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3] * w;
    float yp = mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3] * w;
    float zp = mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3] * w;
    float wp = mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3] * w;

    return glm::vec4(xp, yp, zp, wp);
}

inline glm::vec3 TransformVector(const glm::vec3& vec, const glm::mat4& inMat)
{
    glm::mat4 mat = glm::transpose(inMat);
    float x = vec.x, y = vec.y, z = vec.z;

    return glm::vec3(mat[0][0] * x + mat[0][1] * y + mat[0][2] * z,
                     mat[1][0] * x + mat[1][1] * y + mat[1][2] * z,
                     mat[2][0] * x + mat[2][1] * y + mat[2][2] * z);
}

inline glm::vec3 TransformNormal(const glm::vec3& norm, const glm::mat4& inMat)
{
    return glm::mat3(glm::transpose(glm::inverse(inMat))) * norm;
    glm::mat4 mat = glm::transpose(inMat);
    mat = glm::inverse(mat);

    float x = norm.x, y = norm.y, z = norm.z;

    return glm::vec3(mat[0][0] * x + mat[1][0] * y + mat[2][0] * z,
                     mat[0][1] * x + mat[1][1] * y + mat[2][1] * z,
                     mat[0][2] * x + mat[1][2] * y + mat[2][2] * z);
}

class BBox
{
public:
    BBox()
    {
        mMin = glm::vec3(Infinity, Infinity, Infinity);
        mMax = glm::vec3(NegInfinity, NegInfinity, NegInfinity);
    }

    BBox(const glm::vec3& p)
    {
        mMin = p;
        mMax = p;
    }

    BBox(const glm::vec3& p1, const glm::vec3& p2)
    {
        mMin = glm::vec3(glm::min(p1.x, p2.x), glm::min(p1.y, p2.y), glm::min(p1.z, p2.z));
        mMax = glm::vec3(glm::max(p1.x, p2.x), glm::max(p1.y, p2.y), glm::max(p1.z, p2.z));
    }
    //返回对角线向量
    glm::vec3 diagonal() const
    {
        return (mMax - mMin);
    }

    //求面积
    float surfaceArea() const
    {
        glm::vec3 d = diagonal();
        return (d.x * d.y + d.x * d.z + d.y * d.z) * 2;
    }

    //求体积
    float Volume() const
    {
        glm::vec3 d = diagonal();
        return d.x * d.y * d.z;
    }

    //获取最大的边界
    int maximumExtent() const
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
    const glm::vec3 &operator[](int i) const
    {
        return (&mMin)[i];
    }

    glm::vec3 &operator[](int i)
    {
        return (&mMin)[i];
    }

    glm::vec3 mMin;
    glm::vec3 mMax;
};

inline BBox Union(const BBox& bbox1, const BBox& bbox2)
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

inline BBox Union(const BBox& bbox, const glm::vec3& point)
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