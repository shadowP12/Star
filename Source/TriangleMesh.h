#pragma once
#include "CommonMath.h"
#include <vector>
#include <memory>

class TriangleMesh;

class Triangle
{
public:
    Triangle(std::shared_ptr<TriangleMesh> mesh, int triangleNumber, glm::mat4 objectToWorld);
    ~Triangle();
    BBox objectBound();
    BBox worldBound();
    void getPositionData(glm::vec3& p0, glm::vec3& p1, glm::vec3& p2);
    void getNormalData(glm::vec3& n0, glm::vec3& n1, glm::vec3& n2);
    void getUVData(glm::vec2& t0, glm::vec2& t1, glm::vec2& t2);
    uint32_t getMaterialID();
private:
    std::shared_ptr<TriangleMesh> mMesh;
    int mVertexIndices[3];
    glm::vec3 mNormal;
    glm::mat4 mObjectToWorld, mWorldToObject;
    uint32_t mGlobalIndex;
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Material
{
    glm::vec3 baseColor;
    glm::vec3 emissive;
    float metallic;
    float roughness;
};

struct TriangleSubMesh
{
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    uint32_t materialID = -1;
};

class TriangleMesh
{
public:
    friend class Triangle;
    TriangleMesh()
    {
        mVertexBuffer.resize(0);
        mIndexBuffer.resize(0);
        mSubMeshs.resize(0);
    }
    ~TriangleMesh()
    {
        mVertexBuffer.clear();
        mIndexBuffer.clear();
        mSubMeshs.clear();
    }

public:
    std::vector<Vertex> mVertexBuffer;
    std::vector<uint32_t> mIndexBuffer;
    std::vector<TriangleSubMesh> mSubMeshs;
    uint32_t mIndexCount;
    uint32_t mVertexCount;
    uint32_t mTriangleCount;
    glm::mat4 mWorldMatrix;
};