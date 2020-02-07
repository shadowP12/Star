#pragma once
#include "math/GMath.h"
#include <vector>
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};
class TriangleMesh
{
public:
	TriangleMesh();
	std::vector<Vertex> mVertexBuffer;
	std::vector<uint32_t> mIndexBuffer;
	uint32_t mIndexCount;
	uint32_t mVertexCount;
	uint32_t mTriangleCount;
	glm::mat4 mWorldMatrix;
};
