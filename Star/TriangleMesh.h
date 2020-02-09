#pragma once
#include "math/GMath.h"
#include <vector>
#include <memory>
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
	TriangleMesh();
	~TriangleMesh();

public:
	std::vector<Vertex> mVertexBuffer;
	std::vector<uint32_t> mIndexBuffer;
	std::vector<TriangleSubMesh> mSubMeshs;
	uint32_t mIndexCount;
	uint32_t mVertexCount;
	uint32_t mTriangleCount;
	glm::mat4 mWorldMatrix;
};
