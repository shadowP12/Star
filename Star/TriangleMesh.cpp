#include "TriangleMesh.h"

TriangleMesh::TriangleMesh()
{
    mVertexBuffer.resize(0);
    mIndexBuffer.resize(0);
    mSubMeshs.resize(0);
}

TriangleMesh::~TriangleMesh()
{
    mVertexBuffer.clear();
    mIndexBuffer.clear();
    mSubMeshs.clear();
}