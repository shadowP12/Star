#include "Triangle.h"
#include "DebugDraw.h"

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

bool Triangle::intersect(Ray & r, Intersection & in)
{
	//利用质心坐标
	const glm::vec3& v0 = mMesh->mVertexBuffer[mVertexIndices[0]].pos;
	const glm::vec3& v1 = mMesh->mVertexBuffer[mVertexIndices[1]].pos;
	const glm::vec3& v2 = mMesh->mVertexBuffer[mVertexIndices[2]].pos;

	glm::vec3 e1 = v1 - v0;
	glm::vec3 e2 = v2 - v0;
	glm::vec3 p = glm::cross(r.mDir, e2);
	float a = glm::dot(e1, p);
	if (fabs(a) < 1e-5f)
		return false; // parallel to the plane

	float f = 1.0f / a;
	glm::vec3 s = r.mOrig - v0;
	float u = f * glm::dot(s, p);

	if (u < 0.0f || u > 1.0f)
		return false; // but outside the triangle

	glm::vec3 q = cross(s, e1);
	float v = f * glm::dot(r.mDir, q);

	if (v < 0.0f || (u + v) > 1.0f)
		return false; // but outside the triangle

	float t = f * dot(e2, q);

	if (t > r.mMin && t < r.mMax)
	{
		if (t < in.mDistance)
		{
			in.mDistance = t;
			in.mPos = r.pointAt(t);
			in.mNormal = glm::normalize(TransformNormal(glm::cross(e1, e2), mObjectToWorld));
		}
		return true;
	}
	return false;
}

bool Triangle::intersectP(Ray& r)
{
	const glm::vec3& v0 = mMesh->mVertexBuffer[mVertexIndices[0]].pos;
	const glm::vec3& v1 = mMesh->mVertexBuffer[mVertexIndices[1]].pos;
	const glm::vec3& v2 = mMesh->mVertexBuffer[mVertexIndices[2]].pos;

	glm::vec3 e1 = v1 - v0;
	glm::vec3 e2 = v2 - v0;
	glm::vec3 p = glm::cross(r.mDir, e2);
	float a = glm::dot(e1, p);
	if (fabs(a) < 1e-5f)
		return false; // parallel to the plane

	float f = 1.0f / a;
	glm::vec3 s = r.mOrig - v0;
	float u = f * glm::dot(s, p);

	if (u < 0.0f || u > 1.0f)
		return false; // but outside the triangle

	glm::vec3 q = cross(s, e1);
	float v = f * glm::dot(r.mDir, q);

	if (v < 0.0f || (u + v) > 1.0f)
		return false; // but outside the triangle

	float t = f * dot(e2, q);

	if (t > r.mMin && t < r.mMax)
	{
		return true;
	}
	return false;
}
