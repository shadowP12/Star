#include "Triangle.h"


Triangle::Triangle(std::shared_ptr<TriangleMesh> mesh, int triangleNumber, glm::mat4 objectToWorld)
{
	mMesh = mesh;
	mObjectToWorld = objectToWorld;
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
