#pragma once
#include "TriangleMesh.h"
#include "BBox.h"
#include <memory>
//后期将使用Point还有Normal代替glm::vec3,目的是为了更好区分运算
class Triangle
{
public:
	Triangle(std::shared_ptr<TriangleMesh> mesh, int triangleNumber, glm::mat4 objectToWorld);
	~Triangle();
	BBox objectBound();
	BBox worldBound();
private:
	std::shared_ptr<TriangleMesh> mMesh;
	int mVertexIndices[3];
	glm::vec3 mNormal;
	//从模型空间到世界空间和世界空间到模型空间
	glm::mat4 mObjectToWorld, mWorldToObject;
};
