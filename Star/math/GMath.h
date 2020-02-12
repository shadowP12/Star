#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <math.h>
namespace GMath
{
	static constexpr float Infinity = std::numeric_limits<float>::infinity();
	static constexpr float NegInfinity = -Infinity;
	static constexpr float Epsilon = std::numeric_limits<float>::epsilon();
	static constexpr float Pi = 3.14159265358979323846;
	static constexpr float InvPi = 1.0f / Pi;
	static constexpr float TwoPi = 2.0f * Pi;
	static constexpr float InvTwoPi = 1.0f / TwoPi;
};

inline glm::vec3 TransformPoint(const glm::vec3& point, const glm::mat4& inMat)
{
//    glm::vec4 p = glm::vec4(point.x, point.y, point.z, 1.0f);
//    glm::vec4 r = inMat * p;
//    return glm::vec3(r.x, r.y, r.z);
    glm::mat4 mat = glm::transpose(inMat);
	float x = point.x, y = point.y, z = point.z;
	float xp = mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3];
	float yp = mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3];
	float zp = mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3];
	float wp = mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3];
	assert(wp != 0);

	if (wp == 1.0f)
		return glm::vec3(xp, yp, zp);
	else
		return glm::vec3(xp, yp, zp) / wp;
}

inline glm::vec4 TransformPoint(const glm::vec4& point, const glm::mat4& inMat)
{
    glm::mat4 mat = glm::transpose(inMat);
	float x = point.x, y = point.y, z = point.z, w = point.w;
	float xp = mat[0][0] * x + mat[0][1] * y + mat[0][2] * z + mat[0][3] * w;
	float yp = mat[1][0] * x + mat[1][1] * y + mat[1][2] * z + mat[1][3] * w;
	float zp = mat[2][0] * x + mat[2][1] * y + mat[2][2] * z + mat[2][3] * w;
	float wp = mat[3][0] * x + mat[3][1] * y + mat[3][2] * z + mat[3][3] * w;

	return glm::vec4(xp, yp, zp, wp);
}

inline glm::vec3 TransformVector(const glm::vec3& vec, const glm::mat4& inMat)
{
    glm::mat4 mat = glm::transpose(inMat);
	float x = vec.x, y = vec.y, z = vec.z;

	return glm::vec3(mat[0][0] * x + mat[0][1] * y + mat[0][2] * z,
		mat[1][0] * x + mat[1][1] * y + mat[1][2] * z,
		mat[2][0] * x + mat[2][1] * y + mat[2][2] * z);
}

inline glm::vec3 TransformNormal(const glm::vec3& norm, const glm::mat4& inMat)
{
    return glm::mat3(glm::transpose(glm::inverse(inMat))) * norm;
    glm::mat4 mat = glm::transpose(inMat);
    mat = glm::inverse(mat);

	float x = norm.x, y = norm.y, z = norm.z;

	return glm::vec3(mat[0][0] * x + mat[1][0] * y + mat[2][0] * z,
		mat[0][1] * x + mat[1][1] * y + mat[2][1] * z,
		mat[0][2] * x + mat[1][2] * y + mat[2][2] * z);
}

inline float saturate(float v) { if (v < 0) return 0; if (v > 1) return 1; return v; }

inline glm::vec2 project(const glm::vec4& viewPort, const glm::vec3& point, const glm::mat4& viewProjectionMatrix)
{
	glm::vec4 clipPos;
	clipPos = viewProjectionMatrix * glm::vec4(point.x, point.y, point.z, 1.0);
	float ndcX = clipPos.x / clipPos.w;
	float ndcY = clipPos.y / clipPos.w;
	glm::vec2 out;
	out.x = viewPort.x + (ndcX + 1.0f) * 0.5f * viewPort.z;
	out.y = viewPort.y + (1.0f - (ndcY + 1.0f) * 0.5f) * viewPort.w;
	return out;
}

inline glm::vec3 unProject(const glm::vec4& viewPort, const glm::vec2& point, float depth,const glm::mat4& viewProjectionMatrix)
{
	glm::mat4 inverseViewProjectionMatrix = glm::inverse(viewProjectionMatrix);
	glm::vec4 screen = glm::vec4((point.x - viewPort.x) / viewPort.z, ((viewPort.w - point.y) - viewPort.y) / viewPort.w, depth, 1.0f);

	screen.x = screen.x * 2.0f - 1.0f;
	screen.y = screen.y * 2.0f - 1.0f;
	screen.z = screen.z * 2.0f - 1.0f;

	glm::vec4 worldPoint = inverseViewProjectionMatrix * screen;
	worldPoint.x = worldPoint.x / worldPoint.w;
	worldPoint.y = worldPoint.y / worldPoint.w;
	worldPoint.z = worldPoint.z / worldPoint.w;

	glm::vec3 out = glm::vec3(worldPoint.x, worldPoint.y, worldPoint.z);
	return out;
}
