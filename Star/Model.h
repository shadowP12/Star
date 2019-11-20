#pragma once
#include "Node.h"
#include "GMath.h"
#include "TriangleMesh.h"
#include "glad/glad.h"
#include <string>
class Model : public Node
{
public:
	Model(std::string name, std::shared_ptr<TriangleMesh> mesh);
	~Model();
	void initModel();
	void draw();
	std::shared_ptr<TriangleMesh> getMesh()
	{
		return mMesh;
	}
private:
	std::string mName;
	std::shared_ptr<TriangleMesh> mMesh;
	GLuint mVAO, mVBO, mEBO;
};