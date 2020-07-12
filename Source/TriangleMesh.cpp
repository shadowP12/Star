#include "TriangleMesh.h"

Triangle::Triangle(std::shared_ptr<TriangleMesh> mesh, int triangleNumber, glm::mat4 objectToWorld)
{
    mMesh = mesh;
    mObjectToWorld = mMesh->mWorldMatrix;
    mWorldToObject = glm::inverse(objectToWorld);
    uint32_t* vertexIndices = &(mMesh->mIndexBuffer[triangleNumber * 3]);
    mVertexIndices[0] = vertexIndices[0];
    mVertexIndices[1] = vertexIndices[1];
    mVertexIndices[2] = vertexIndices[2];

    glm::vec3 edge0 = mMesh->mVertexBuffer[mVertexIndices[1]].pos - mMesh->mVertexBuffer[mVertexIndices[0]].pos;
    glm::vec3 edge1 = mMesh->mVertexBuffer[mVertexIndices[2]].pos - mMesh->mVertexBuffer[mVertexIndices[1]].pos;
    mNormal = glm::normalize(glm::cross(edge0, edge1));
}

Triangle::~Triangle()
{
}

BBox Triangle::objectBound()
{
    const glm::vec3& p0 = mMesh->mVertexBuffer[mVertexIndices[0]].pos;
    const glm::vec3& p1 = mMesh->mVertexBuffer[mVertexIndices[1]].pos;
    const glm::vec3& p2 = mMesh->mVertexBuffer[mVertexIndices[2]].pos;

    return Union(BBox(p0, p1), p2);
}

BBox Triangle::worldBound()
{
    const glm::vec3& p0 = TransformPoint(mMesh->mVertexBuffer[mVertexIndices[0]].pos, mObjectToWorld);
    const glm::vec3& p1 = TransformPoint(mMesh->mVertexBuffer[mVertexIndices[1]].pos, mObjectToWorld);
    const glm::vec3& p2 = TransformPoint(mMesh->mVertexBuffer[mVertexIndices[2]].pos, mObjectToWorld);

    return Union(BBox(p0, p1), p2);
}

void Triangle::getPositionData(glm::vec3& p0, glm::vec3& p1, glm::vec3& p2)
{
    p0 = TransformPoint(mMesh->mVertexBuffer[mVertexIndices[0]].pos, mMesh->mWorldMatrix);
    p1 = TransformPoint(mMesh->mVertexBuffer[mVertexIndices[1]].pos, mMesh->mWorldMatrix);
    p2 = TransformPoint(mMesh->mVertexBuffer[mVertexIndices[2]].pos, mMesh->mWorldMatrix);
}

void Triangle::getNormalData(glm::vec3& n0, glm::vec3& n1, glm::vec3& n2)
{
    n0 = TransformNormal(mMesh->mVertexBuffer[mVertexIndices[0]].normal, mMesh->mWorldMatrix);
    n1 = TransformNormal(mMesh->mVertexBuffer[mVertexIndices[1]].normal, mMesh->mWorldMatrix);
    n2 = TransformNormal(mMesh->mVertexBuffer[mVertexIndices[2]].normal, mMesh->mWorldMatrix);
}

void Triangle::getUVData(glm::vec2 &t0, glm::vec2 &t1, glm::vec2 &t2)
{
    t0 = mMesh->mVertexBuffer[mVertexIndices[0]].uv;
    t1 = mMesh->mVertexBuffer[mVertexIndices[1]].uv;
    t2 = mMesh->mVertexBuffer[mVertexIndices[2]].uv;
}

uint32_t Triangle::getMaterialID()
{
    uint32_t id = -1;
    for (int i = 0; i < mMesh->mSubMeshs.size(); ++i)
    {
        TriangleSubMesh& subMesh = mMesh->mSubMeshs[i];
        if(mVertexIndices[0] >= mMesh->mSubMeshs[i].indexOffset && mVertexIndices[0] <= mMesh->mSubMeshs[i].indexOffset + mMesh->mSubMeshs[i].indexCount)
        {
            id = mMesh->mSubMeshs[i].materialID;
            break;
        }
    }
    return id;
}
