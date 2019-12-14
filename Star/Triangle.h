#pragma once
#include "TriangleMesh.h"
#include "BBox.h"
#include "Ray.h"
#include "Intersection.h"
#include <memory>
//后期将使用Point还有Normal代替glm::vec3,目的是为了更好区分运算
//注意:现在Triangle内顶点的坐标系依旧是在模型坐标系,但是由于没有做额外的变换所以和世界坐标系是重叠的
class Triangle
{
public:
	Triangle(std::shared_ptr<TriangleMesh> mesh, int triangleNumber, glm::mat4 objectToWorld);
	~Triangle();
	BBox objectBound();
	BBox worldBound();
	bool intersect(Ray& r, Intersection& in);
	bool intersectP(Ray& r);
	void getVertexData(glm::vec3& p0, glm::vec3& p1, glm::vec3& p2);
private:
	std::shared_ptr<TriangleMesh> mMesh;
	int mVertexIndices[3];
	glm::vec3 mNormal;
	//从模型空间到世界空间和世界空间到模型空间
	glm::mat4 mObjectToWorld, mWorldToObject;
};
