#include "Model.h"
#include <string>
Model::Model(std::string name, std::shared_ptr<TriangleMesh> mesh)
{
	mName = name;
	mMesh = mesh;
}

Model::~Model()
{
}

void Model::initModel()
{
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mEBO);

	glBindVertexArray(mVAO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);

	glBufferData(GL_ARRAY_BUFFER, mMesh->mVertexBuffer.size() * sizeof(Vertex), &mMesh->mVertexBuffer[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mMesh->mIndexBuffer.size() * sizeof(uint32_t), &mMesh->mIndexBuffer[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

	glBindVertexArray(0);
}

void Model::draw()
{
	glBindVertexArray(mVAO);
	glDrawElements(GL_TRIANGLES, mMesh->mIndexBuffer.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}
